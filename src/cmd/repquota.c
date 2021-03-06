/*****************************************************************************\
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
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
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
#include <sys/stat.h>

#include "src/libutil/getconf.h"
#include "src/libutil/util.h"
#include "src/libutil/listint.h"

#include "getquota.h"

static void usage(void);
static void add_quota(confent_t *cp, List qlist, uid_t uid, char *name);
static void dirscan(confent_t *conf, List qlist, List uids, int getusername);
static void pwscan(confent_t *conf, List qlist, List uids, int getusername);
static void uidscan(confent_t *conf, List qlist, List uids, int getusername);

char *prog;
int debug = 0;

extern double quota_nfs_timeout;
extern double quota_nfs_retry_timeout;

#define OPTIONS "u:b:dHrsFf:UpTDnhN:R:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"dirscan",          no_argument,        0, 'd'},
    {"pwscan",           no_argument,        0, 'p'},
    {"blocksize",        required_argument,  0, 'b'},
    {"uid-range",        required_argument,  0, 'u'},
    {"reverse",          no_argument,        0, 'r'},
    {"space-sort",       no_argument,        0, 's'},
    {"files-sort",       no_argument,        0, 'F'},
    {"usage-only",       no_argument,        0, 'U'},
    {"suppress-heading", no_argument,        0, 'H'},
    {"config",           required_argument,  0, 'f'},
    {"selftest",         no_argument,        0, 'T'},
    {"debug",            no_argument,        0, 'D'},
    {"nouserlookup",     no_argument,        0, 'n'},
    {"human-readable",   no_argument,        0, 'h'},
    {"nfs-timeout",      required_argument,  0, 'N'},
    {"nfs-retry-timeout",required_argument,  0, 'R'},

    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    confent_t *conf;
    int c;
    int dopt = 0;
    int popt = 0;
    unsigned long bsize = 1024*1024;
    char *fsname = NULL;
    List qlist;
    int Fopt = 0;
    int ropt = 0;
    int sopt = 0;
    int Hopt = 0;
    int Uopt = 0;
    int nopt = 0;
    int hopt = 0;
    List uids = NULL;
    char *conf_path = _PATH_QUOTA_CONF;
    conf_t config;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch(c) {
            case 'd':   /* --dirscan */
                dopt++;
                break;
            case 'p':   /* --pwscan */
                popt++;
                break;
            case 'b':   /* --blocksize */
                if (parse_blocksize(optarg, &bsize)) {
                    fprintf(stderr, "%s: error parsing blocksize\n", prog);
                    exit(1);
                }
                break;
            case 'u':   /* --uid-range */
                uids = listint_create(optarg);
                if (uids == NULL) {
                    fprintf(stderr, "%s: error parsing uid-range\n", prog);
                    exit(1);
                }
                break;
            case 'r':   /* --reverse */
                ropt++;
                break;
            case 'F':   /* --files-sort */
                Fopt++;
                break;
            case 's':   /* --space-sort */
                sopt++;
                break;
            case 'U':   /* --usage-only */
                Uopt++;
                break;
            case 'H':   /* --suppress-heading */
                Hopt++;
                break;
            case 'f':   /* --config */
                conf_path = optarg;
                break;
            case 'T':   /* --selftest */
#ifndef NDEBUG
                listint_test();
                exit(0);
#else
                fprintf(stderr, "%s: not built with debugging enabled\n", prog);
                exit(1);
#endif
                break;
            case 'D':   /* --debug */
                debug = 1;
                break;
            case 'n':   /* --nouserlookup */
                nopt = 1;
                break;
            case 'h':   /* --human-readable */
                hopt = 1;
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
    if (Fopt && sopt) {
        fprintf(stderr, "%s: -f and -s are mutually exclusive\n", prog);
        exit(1);
    }
    if (popt && dopt) {
        fprintf(stderr, "%s: -p and -d are mutually exclusive\n", prog);
        exit(1);
    }
    if (!popt && !dopt && !uids) {
        fprintf(stderr, "%s: need at least one of -pdu\n", prog);
        exit(1);
    }
    if (optind < argc)
        fsname = argv[optind++];
    else
        usage();
    if (optind < argc)
        usage();
    config = conf_init(conf_path); /* exit/perror on error */

    if (!(conf = conf_get_bylabel(config, fsname, 0))) {
        fprintf(stderr, "%s: %s: not found in quota.conf\n", prog, fsname);
        exit(1);
    }

    /* Scan.
     */
    qlist = list_create((ListDelF)quota_destroy);
    if (popt)
        pwscan(conf, qlist, uids, !nopt);
    if (dopt)
        dirscan(conf, qlist, uids, !nopt);
    if (!dopt && !popt)
        uidscan(conf, qlist, uids, !nopt);


    /* Sort.
     */
    if (ropt) {
        if (sopt)
            list_sort(qlist, (ListCmpF)quota_cmp_bytes_reverse);
        else if (Fopt)
            list_sort(qlist, (ListCmpF)quota_cmp_files_reverse);
        else
            list_sort(qlist, (ListCmpF)quota_cmp_uid_reverse);
    } else {
        if (sopt)
            list_sort(qlist, (ListCmpF)quota_cmp_bytes);
        else if (Fopt)
            list_sort(qlist, (ListCmpF)quota_cmp_files);
        else
            list_sort(qlist, (ListCmpF)quota_cmp_uid);
    }

    /* Report.
     */
    if (!Hopt) {

        if (hopt)
            printf("Quota report for %s\n", fsname);
        else {
            char tmpstr[16];
            size2str(bsize, tmpstr, sizeof(tmpstr));
            printf("Quota report for %s (blocksize %s)\n", fsname, tmpstr);
        }
    }
    if (Uopt) {
        if (!Hopt)
            quota_report_heading_usageonly();
        if (hopt)
            list_for_each(qlist, (ListForF)quota_report_usageonly_h, &bsize);
        else
            list_for_each(qlist, (ListForF)quota_report_usageonly, &bsize);
    } else {
        if (!Hopt)
            quota_report_heading();
        if (hopt)
            list_for_each(qlist, (ListForF)quota_report_h, &bsize);
        else
            list_for_each(qlist, (ListForF)quota_report, &bsize);
    }

    if (qlist)
        list_destroy(qlist);
    if (uids)
        listint_destroy(uids);
    conf_fini(config);

    return 0;
}

static void
usage(void)
{
    fprintf(stderr,
  "Usage: %s [--options] fs\n"
  "  -d,--dirscan           report on users who own top level dirs of fs\n"
  "  -p,--pwscan            report on users in the password file\n"
  "  -b,--blocksize         report usage in blocksize units (default 1M)\n"
  "  -u,--uid-range         set range/list of uid's to include in report\n"
  "  -r,--reverse-sort      sort in reverse order\n"
  "  -s,--space-sort        sort on space used (default sort on uid)\n"
  "  -F,--files-sort        sort on files used (default sort on uid)\n"
  "  -U,--usage-only        only report usage, not quota limits\n"
  "  -H,--suppress-heading  suppress report heading\n"
  "  -L,--nolimits          do not include quota limits in report\n"
  "  -n,--nouserlookup      do not try to map uid's to user names\n"
  "  -f,--config            use a config file other than %s\n"
  "  -N,--nfs-timeout=SEC   set per filesystem NFS timeout (%.2fs default)\n"
  "  -R,--nfs-retry-timeout=SEC    set NFS retry timeout (%.2fs default)\n"
                , prog, _PATH_QUOTA_CONF,
                quota_nfs_timeout,
                quota_nfs_retry_timeout);
    exit(1);
}

/* Query the quota for uid and add it to qlist if successful.
 */
static void
add_quota(confent_t *cp, List qlist, uid_t uid, char *name)
{
    quota_t q;

    if (list_find_first(qlist, (ListFindF)quota_match_uid, &uid))
        return;
    q = quota_create(cp->cf_label, cp->cf_rhost, cp->cf_rpath, cp->cf_thresh);
    if (quota_get(uid, q)) {
        quota_destroy(q);
        return;
    }
    if (name)
        quota_adduser (q, name);
    list_append(qlist, q);
}

/* Get quotas for all uid's in uids list.
 */
static void
uidscan(confent_t *cp, List qlist, List uids, int getusername)
{
    struct passwd *pw;
    ListIterator itr;
    quota_t q;
    unsigned long *up;
    int rc;
    char name[32];

    itr = list_iterator_create(uids);
    while ((up = list_next(itr))) {
        if (getusername) {
            if ((pw = getpwuid ((uid_t)*up)))
                snprintf (name, sizeof(name), "%s", pw->pw_name);
            else
                snprintf (name, sizeof(name), "[%lu]", *up);
            add_quota(cp, qlist, pw->pw_uid, name);
        } else
            add_quota(cp, qlist, (uid_t)*up, NULL);
    }
    list_iterator_destroy(itr);
}

/* Get quotas for all owners of top-level directories, optionally
 * filtered by uids.
 */
static void
dirscan(confent_t *cp, List qlist, List uids, int getusername)
{
    struct passwd *pw;
    struct dirent *dp;
    DIR *dir;
    char fqp[MAXPATHLEN];
    struct stat sb;
    quota_t q;
    char name[32];

    if (!(dir = opendir(cp->cf_rpath))) {
        fprintf(stderr, "%s: could not open %s\n", prog, cp->cf_rpath);
        exit(1);
    }
    while ((dp = readdir(dir))) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        snprintf(fqp, sizeof(fqp), "%s/%s", cp->cf_rpath, dp->d_name);
        if (stat(fqp, &sb) < 0)
            continue;
        if (uids && !listint_member(uids, sb.st_uid))
            continue;
        if (getusername) {
            if ((pw = getpwuid(sb.st_uid)))
                snprintf (name, sizeof(name), "%s", pw->pw_name);
            else
                snprintf (name, sizeof(name), "[%.*s]",
                          (int)sizeof (name) - 3, dp->d_name);
            add_quota(cp, qlist, sb.st_uid, name);
        } else
            add_quota(cp, qlist, sb.st_uid, NULL);
    }
    if (closedir(dir) < 0)
        fprintf(stderr, "%s: closedir %s: %m\n", prog, cp->cf_rpath);
}

/* Get quotas for all users in the password file, optionally filtered
 * by uids list.
 */
static void
pwscan(confent_t *cp, List qlist, List uids, int getusername)
{
    struct passwd *pw;
    quota_t q;

    while ((pw = getpwent()) != NULL) {
        if (uids && !listint_member(uids, pw->pw_uid))
            continue;
        add_quota(cp, qlist, pw->pw_uid, getusername ? pw->pw_name : NULL);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
