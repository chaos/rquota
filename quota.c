/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-005.
 *  
 *  This file is part of Quota, a remote quota program.
 *  For details, see <http://www.llnl.gov/linux/quota/>.
 *  
 *  Quota is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  Quota is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with Quota; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#include <ctype.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>          /* MAXHOSTNAMELEN */
#include <signal.h>
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

#include "getconf.h"
#include "getquota.h"
#include "util.h"
#include "list.h"

static void usage(void);
static void alarm_handler(int arg);
static void lookup_user(char **user, uid_t *uid, char **dir);
static void get_login_quota(char *homedir, uid_t uid, List qlist);
static void get_all_quota(uid_t uid, List qlist);

char *prog;

int 
main(int argc, char *argv[])
{
    int vopt = 0, ropt = 0, lopt = 0;
    char *user = NULL;
    char *dir = NULL;
    uid_t uid;
    int c;
    extern char *optarg;
    extern int optind;
    List qlist;

    /* handle args */
    prog = basename(argv[0]);
    while ((c = getopt(argc, argv, "f:rvlt:T")) != EOF) {
        switch (c) {
        case 'l':              /* -l display home directory quota only */
            lopt = 1;
            break;
        case 't':               /* -t set timeout in seconds */
            signal(SIGALRM, alarm_handler);
            alarm(strtoul(optarg, NULL, 10));
            break;
        case 'r':              /* -r display rmt mntpt, not descriptive name */
            ropt = 1;
            break;
        case 'v':              /* -v show all quota info for selected fs's */
            vopt = 1;
            break;
        case 'f':              /* -f use alternate config file */
            setconfent(optarg); /* perror/exit on error */
            break;
        case 'T':              /* -T (undocumented) internal unit tests */
#ifndef NDEBUG
            test_match_path();
            exit(0);
#else
            fprintf(stderr, "%s: compiled with -DNDEBUG\n", prog);
            exit(1);
#endif
        default:
            usage();
        }
    }
    if (optind < argc)
        user = xstrdup(argv[optind++]);
    if (optind < argc)
        usage();

    lookup_user(&user, &uid, &dir);
    if (lopt && !dir) {
        fprintf(stderr, "%s: no such user %s\n", prog, user);
        exit(1);
    }

    /* 2>&1 so any errors interrupt the report in a predictable way */
    if (close(2) < 0) {
        fprintf(stderr, "%s: close stderr: %s\n", prog, strerror(errno));
        exit(1);
    }
    if (dup(1) < 0) {
        fprintf(stderr, "%s: dup stdout: %s\n", prog, strerror(errno));
        exit(1);
    }

    /* build list of quotas */
    qlist = list_create((ListDelF)quota_destroy);
    if (lopt)
        get_login_quota(dir, uid, qlist);
    else
        get_all_quota(uid, qlist);

    /* print output */   
    if (vopt) {
        quota_print_heading(user);
        if (ropt)
            list_for_each(qlist, (ListForF)quota_print_realpath, NULL);
        else
            list_for_each(qlist, (ListForF)quota_print, NULL);
    } else {
        if (ropt)
            list_for_each(qlist, (ListForF)quota_print_justwarn_realpath, NULL);
        else
            list_for_each(qlist, (ListForF)quota_print_justwarn, NULL);
    }

    list_destroy(qlist);
    if (user)
        free(user);
    if (dir)
        free(dir);
    alarm(0);
    exit(0);
}

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s [-vlr] [-t sec] [-f conffile] [user]\n", prog);
    exit(1);
}

static void 
alarm_handler(int arg)
{
    fprintf(stderr, "%s: timeout, aborting\n", prog);
    exit(1);
}

static void
lookup_user(char **user, uid_t *uid, char **dir)
{
    struct passwd *pw;

    assert(uid != NULL);
    assert(dir != NULL);
    assert(*dir == NULL);

    if (!*user) {
        *uid = geteuid();
        pw = getpwuid(*uid);
        if (!pw) {
            fprintf(stderr, "%s: getpwuid on geteuid failed!\n", prog);
            exit(1);
        }
        *dir = xstrdup(pw->pw_dir);
        *user = xstrdup(pw->pw_name);
    } else {
        if (isdigit(**user)) {
            *uid = strtoul(*user, NULL, 0);
            pw = getpwuid(*uid);
            if (pw)
                *dir = xstrdup(pw->pw_dir);
        } else {
            pw = getpwnam(*user);
            if (!pw) {
                fprintf(stderr, "%s: no such user: %s\n", prog, *user);
                exit(1);
            }
            *uid = pw->pw_uid;
            *dir = xstrdup(pw->pw_dir);
        } 
    }
}

static void
get_login_quota(char *homedir, uid_t uid, List qlist)
{
    confent_t *cp;
    quota_t q;
    
    if ((cp = getconflabelsub(homedir)) == NULL) {
        fprintf(stderr, "%s: could not find quota.conf entry for %s\n",
                prog, homedir);
        exit(1);
    }
    q = quota_create(cp->cf_label, cp->cf_rhost, cp->cf_rpath, cp->cf_thresh);
    if (quota_get(uid, q)) {
        quota_destroy(q);
        exit(1);
    }
    list_append(qlist, q);
}

static void
get_all_quota(uid_t uid, List qlist)
{
    confent_t *cp;
    quota_t q;

    while ((cp = getconfent()) != NULL) {
        q = quota_create(cp->cf_label, cp->cf_rhost, cp->cf_rpath, 
                          cp->cf_thresh);
        if (quota_get(uid, q)) {
            quota_destroy(q);
            continue; /* keep going and get the rest */
        }
        list_append(qlist, q);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
