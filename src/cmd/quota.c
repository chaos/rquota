/*****************************************************************************\
 *  $Id: quota.c 186 2009-12-14 18:58:09Z garlick $
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

#include "src/libutil/getconf.h"
#include "src/libutil/util.h"

#include "getquota.h"

static void usage(void);
static void alarm_handler(int arg);
static void lookup_user_byname(char *user, uid_t *uidp, char **dirp);
static void lookup_user_byuid(char *user, uid_t *uidp, char **dirp);
static void lookup_self(char **userp, uid_t *uidp, char **dirp);
static void get_login_quota(conf_t config, char *homedir, uid_t uid,
                            List qlist, int skipnolimit);
static void get_all_quota(conf_t config, uid_t uid, List qlist,
                          int skipnolimit);

#define OPTIONS "f:rvlt:TdN:R:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"login",            no_argument,        0, 'l'},
    {"timeout",          required_argument,  0, 't'},
    {"realpath",         no_argument,        0, 'r'},
    {"verbose",          no_argument,        0, 'v'},
    {"config",           required_argument,  0, 'f'},
    {"selftest",         no_argument,        0, 'T'},
    {"debug",            no_argument,        0, 'd'},
    {"nfs-timeout",      required_argument,  0, 'N'},
    {"nfs-retry-timeout",required_argument,  0, 'R'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

char *prog;
int debug = 0;

extern double quota_nfs_timeout;
extern double quota_nfs_retry_timeout;

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
    char *conf_path = _PATH_QUOTA_CONF;
    conf_t config = NULL;

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
            conf_path = optarg;
            break;
        case 'T':   /* --selftest (undocumented) */
#ifndef NDEBUG
            test_match_path();
            exit(0);
#else
            fprintf(stderr, "%s: compiled with -DNDEBUG\n", prog);
            exit(1);
#endif
        case 'd':   /* --debug (undocumented) */
            debug = 1;
            break;
        case 'N':   /* --nfs-timeout SECS */
            quota_nfs_timeout = strtod (optarg, NULL);
            break;
        case 'R':   /* --nfs-retry-timeout SECS */
            quota_nfs_retry_timeout = strtod (optarg, NULL);
            break;
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

    config = conf_init(conf_path); /* exit/perror on error */

    /* build list of quotas */
    qlist = list_create((ListDelF)quota_destroy);
    if (lopt)
        get_login_quota(config, dir, uid, qlist, !vopt);
    else
        get_all_quota(config, uid, qlist, !vopt);

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
    conf_fini(config);
    alarm(0);
    exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-vlr] [-t sec] [-N sec] [-R sec] [-f conffile] [user]\n", prog);
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
     * on password file content.
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
get_login_quota(conf_t config, char *homedir, uid_t uid, List qlist,
                int skipnolimit)
{
    confent_t *cp;
    quota_t q;

    if ((cp = conf_get_bylabel(config, homedir, CONF_MATCH_SUBDIR)) == NULL) {
        fprintf(stderr, "%s: could not find quota.conf entry for %s\n",
                prog, homedir);
        exit(1);
    }
    if (skipnolimit && cp->cf_nolimit)
        return;
    q = quota_create(cp->cf_label, cp->cf_rhost, cp->cf_rpath, cp->cf_thresh);
    if (quota_get(uid, q)) {
        quota_destroy(q);
        exit(1);
    }
    list_append(qlist, q);
}

static void
get_all_quota(conf_t config, uid_t uid, List qlist, int skipnolimit)
{
    confent_t *cp;
    quota_t q;
    conf_iterator_t itr;

    itr = conf_iterator_create(config);
    while ((cp = conf_next(itr)) != NULL) {
        if (skipnolimit && cp->cf_nolimit)
            continue;
        q = quota_create(cp->cf_label, cp->cf_rhost, cp->cf_rpath,
                          cp->cf_thresh);
        if (quota_get(uid, q)) {
            quota_destroy(q);
            continue; /* keep going and get the rest */
        }
        list_append(qlist, q);
    }
    conf_iterator_destroy(itr);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
