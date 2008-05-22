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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <ctype.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>          /* MAXHOSTNAMELEN */
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <signal.h>
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

#include "getconf.h"
#include "getquota.h"
#include "list.h"
#include "util.h"

static void usage(void);
static void alarm_handler(int arg);
static void lookup_user_byname(char *user, uid_t *uidp, char **dirp);
static void lookup_user_byuid(char *user, uid_t *uidp, char **dirp);
static void lookup_self(char **userp, uid_t *uidp, char **dirp);
static void get_login_quota(char *homedir, uid_t uid, List qlist);
static void get_all_quota(uid_t uid, List qlist);

#define OPTIONS "f:rvlt:T"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"login",            no_argument,        0, 'l'},
    {"timeout",          required_argument,  0, 't'},
    {"realpath",         no_argument,        0, 'r'},
    {"verbose",          no_argument,        0, 'v'},
    {"config",           required_argument,  0, 'f'},
    {"selftest",         no_argument,        0, 'T'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

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
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'l':   /* --login */
            lopt = 1;
            break;
        case 't':   /* --timeout */
            signal(SIGALRM, alarm_handler);
            alarm(strtoul(optarg, NULL, 10));
            break;
        case 'r':   /* --realpath */
            ropt = 1;
            break;
        case 'v':   /* --verbose */
            vopt = 1;
            break;
        case 'f':   /* --config file */
            setconfent(optarg); /* perror/exit on error */
            break;
        case 'T':   /* --selftest (undocumented) */
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

    if (!user)
        lookup_self(&user, &uid, &dir);
    else if (isdigit(*user))
        lookup_user_byuid(user, &uid, &dir);
    else
        lookup_user_byname(user, &uid, &dir);

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
        int i = 0;

        if (ropt)
            list_for_each(qlist, (ListForF)quota_print_justwarn_realpath, &i);
        else
            list_for_each(qlist, (ListForF)quota_print_justwarn, &i);
        if (i > 0)
            printf("Run quota -v for more detailed information.\n");
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
lookup_self(char **userp, uid_t *uidp, char **dirp)
{
    struct passwd *pw = getpwuid(geteuid());

    if (!pw) {
        fprintf(stderr, "%s: getpwuid on geteuid failed!\n", prog);
        exit(1);
    }
    *uidp = pw->pw_uid;
    *dirp = xstrdup(pw->pw_dir);
    *userp = xstrdup(pw->pw_name);
}

static void
lookup_user_byuid(char *user, uid_t *uidp, char **dirp)
{
    struct passwd *pw;
    char *endptr;
    uid_t uid = strtoul(user, &endptr, 10);
    
    if (*endptr != '\0') {
        fprintf(stderr, "%s: error parsing uid\n", prog);
        exit(1);
    }
    pw = getpwuid(uid);
    if (pw) {
    	*uidp = pw->pw_uid;
        *dirp = xstrdup(pw->pw_dir);
    /* N.B. Passwd lookup failure of numerical user arg is not fatal.
     * In testing, we pass in arbitrary uid's to trigger hardwired response
     * from getquota_test(), and we don't want test results to be dependent 
     * on password file conteint.
     */
    } else { 
        *uidp = uid;
        *dirp = xstrdup("/");
    }
}

static void
lookup_user_byname(char *user, uid_t *uidp, char **dirp)
{
    struct passwd *pw = getpwnam(user);

    if (!pw) {
        fprintf(stderr, "%s: no such user: %s\n", prog, user);
        exit(1);
    }
    *uidp = pw->pw_uid;
    *dirp = xstrdup(pw->pw_dir);
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
