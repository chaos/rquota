/*****************************************************************************\
 *  $Id: quota.c 7324 2008-05-02 14:44:44Z garlick $
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
#include <getopt.h>
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

#include "getconf.h"
#include "getquota.h"
#include "util.h"
#include "list.h"

static void usage(void);
static quota_t *quota_create(void);
static void quota_destroy(quota_t *qp);
static int quota_match_uid(quota_t *x, uid_t *key);
static int quota_cmp_uid(quota_t *x, quota_t *y);
static int quota_cmp_uid_reverse(quota_t *x, quota_t *y);
static int quota_cmp_bytes(quota_t *x, quota_t *y);
static int quota_cmp_bytes_reverse(quota_t *x, quota_t *y);
static int quota_cmp_files(quota_t *x, quota_t *y);
static int quota_cmp_files_reverse(quota_t *x, quota_t *y);
static int quota_print(quota_t *x, unsigned long long *bsize);
static void dirscan(confent_t *conf, List qlist, uid_t minuid, 
                    uid_t maxuid, char *fsname);
static void pwscan(confent_t *conf, List qlist, uid_t minuid, 
                    uid_t maxuid, char *fsname);
static void quota_print_heading(void);

static char *prog;

#define OPTIONS "m:M:b:dhHrsf"
static struct option longopts[] = {
    {"dirscan",          no_argument,        0, 'd'},
    {"blocksize",        required_argument,  0, 'b'},
    {"min-uid",          required_argument,  0, 'm'},
    {"max-uid",          required_argument,  0, 'M'},
    {"reverse",          no_argument,        0, 'r'},
    {"space-sort",       no_argument,        0, 's'},
    {"files-sort",       no_argument,        0, 'f'},
    {"suppress-heading", no_argument,        0, 'H'},
    {0, 0, 0, 0},
};

int 
main(int argc, char *argv[])
{
    confent_t *conf;
    int c;
    int dopt = 0;
    unsigned long bsize = 1024*1024;
    uid_t minuid = 0;
    uid_t maxuid = -1;
    char *fsname;
    List qlist;
    int fopt = 0;
    int ropt = 0;
    int sopt = 0;
    int Hopt = 0;

    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch(c) {
            case 'd':   /* --dirscan */
                dopt++;
                break;
            case 'b':   /* --blocksize */
                if (parse_blocksize(optarg, &bsize)) {
                    fprintf(stderr, "%s: error parsing blocksize\n", prog); 
                    exit(1);
                }
                break;
            case 'm':   /* --min-uid */
                minuid = strtoul(optarg, NULL, 10);
                break;
            case 'M':   /* --max-uid */
                maxuid = strtoul(optarg, NULL, 10);
                break;
            case 'r':   /* --reverse */
                ropt++;
                break;
            case 'f':   /* --files-sort */
                fopt++;
                break;
            case 's':   /* --space-sort */
                sopt++;
                break;
            case 'H':   /* --suppress-heading */
                Hopt++;
                break;
            case 'h':
            default:
                usage();
        }
    }
    if (fopt && sopt) {
        fprintf(stderr, "%s: -f and -s are mutually exclusive\n", prog);
        exit(1);
    }
    if (optind < argc)
        fsname = argv[optind++];
    else
        usage();
    if (optind < argc)
        usage();
    if (!(conf = getconfdesc(fsname))) {
        fprintf(stderr, "%s: %s: not found in quota.conf\n", prog, fsname);
        exit(1);
    }

    /* Scan.
     */
    qlist = list_create((ListDelF)quota_destroy);
    if (!qlist) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    if (dopt) 
        dirscan(conf, qlist, minuid, maxuid, fsname);
    else
        pwscan(conf, qlist, minuid, maxuid, fsname);


    /* Sort.
     */
    if (ropt) {
        if (sopt)
            list_sort(qlist, (ListCmpF)quota_cmp_bytes_reverse);
        else if (fopt)
            list_sort(qlist, (ListCmpF)quota_cmp_files_reverse);
        else
            list_sort(qlist, (ListCmpF)quota_cmp_uid_reverse);
    } else {
        if (sopt)
            list_sort(qlist, (ListCmpF)quota_cmp_bytes);
        else if (fopt)
            list_sort(qlist, (ListCmpF)quota_cmp_files);
        else
            list_sort(qlist, (ListCmpF)quota_cmp_uid);
    }

    /* Report.
     */
    if (!Hopt) {
        printf("Quota report for %s (blocksize %lu bytes)\n",
                fsname, bsize);
        quota_print_heading();
    }
    list_for_each(qlist, (ListForF)quota_print, &bsize);

    list_destroy(qlist);

    return 0;
}

static void
usage(void)
{
    fprintf(stderr, 
  "Usage: %s [--options] fs\n"
  "  -d,--dirscan          report on users who own top level dirs of fs\n"
  "  -b,--blocksize         report usage in blocksize units (default 1m)\n"
  "  -m,--min-uid           set minimum uid to include in report\n"
  "  -M,--max-uid           set maximum uid to include in report\n"
  "  -r,--reverse-sort      sort in reverse order\n"
  "  -s,--space-sort        sort on space used (default sort on uid)\n"
  "  -f,--files-sort        sort on files used (default sort on uid)\n"
  "  -H,--suppress-heading  do not include heading in report\n"
                , prog);
    exit(1);
}

static quota_t *
quota_create(void)
{
    quota_t *qp = xmalloc(sizeof(quota_t));
    qp->q_magic = QUOTA_MAGIC;
    return qp;
}

static void
quota_destroy(quota_t *qp)
{
    assert(qp->q_magic == QUOTA_MAGIC);
    qp->q_magic = 0;
    free(qp);
}

static int
quota_match_uid(quota_t *x, uid_t *key)
{
    assert(x->q_magic == QUOTA_MAGIC);

    return (x->q_uid == *key);
}

static int
quota_cmp_uid(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_uid < y->q_uid)
        return -1;
    if (x->q_uid == y->q_uid)
        return 0;
    return 1;
}

static int
quota_cmp_uid_reverse(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_uid < y->q_uid)
        return 1;
    if (x->q_uid == y->q_uid)
        return 0;
    return -1;
}

static int
quota_cmp_bytes(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_bytes_used < y->q_bytes_used)
        return -1;
    if (x->q_bytes_used == y->q_bytes_used)
        return 0;
    return 1;
}

static int
quota_cmp_bytes_reverse(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_bytes_used < y->q_bytes_used)
        return 1;
    if (x->q_bytes_used == y->q_bytes_used)
        return 0;
    return -1;
}

static int
quota_cmp_files(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_files_used < y->q_files_used)
        return -1;
    if (x->q_files_used == y->q_files_used)
        return 0;
    return 1;
}

static int
quota_cmp_files_reverse(quota_t *x, quota_t *y)
{
    assert(x->q_magic == QUOTA_MAGIC);
    assert(y->q_magic == QUOTA_MAGIC);

    if (x->q_files_used < y->q_files_used)
        return 1;
    if (x->q_files_used == y->q_files_used)
        return 0;
    return -1;
}

static void
quota_print_heading(void)
{
    printf("%-10s%-11s%-11s%-11s %-12s%-12s%-12s\n", "User", 
        "Space-used", "Space-soft", "Space-hard",
        "Files-used", "Files-soft", "Files-hard");
}

static int
quota_print(quota_t *x, unsigned long long *bsize)
{
    struct passwd *pw;
    char *name, tmp[16];

    assert(x->q_magic == QUOTA_MAGIC);
    if ((pw = getpwuid(x->q_uid)))
        name = pw->pw_name;
    else {
        snprintf(tmp, sizeof(tmp), "%u", x->q_uid);
        name = tmp;
    }
    printf("%-10s%-11llu%-11llu%-11llu %-12llu%-12llu%-12llu\n", name, 
        x->q_bytes_used / *bsize,
        x->q_bytes_softlim / *bsize,
        x->q_bytes_hardlim / *bsize,
        x->q_files_used,
        x->q_files_softlim,
        x->q_files_hardlim);
    return 0;
}

static void
dirscan(confent_t *conf, List qlist, uid_t minuid, uid_t maxuid, char *fsname)
{
    struct dirent *dp;
    DIR *dir;
    char fqp[MAXPATHLEN];
    struct stat sb;
    quota_t *q;

    if (!(dir = opendir(conf->cf_path))) {
        fprintf(stderr, "%s: could not open %s\n", prog, conf->cf_path);
        exit(1);
    }
    while ((dp = readdir(dir))) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        snprintf(fqp, sizeof(fqp), "%s/%s", conf->cf_path, dp->d_name);
        if (stat(fqp, &sb) < 0)
            continue;
        if (minuid != 0 && sb.st_uid < minuid)
            continue;
        if (maxuid != -1 && sb.st_uid > maxuid)
            continue;
        if (list_find_first(qlist, (ListFindF)quota_match_uid, &sb.st_uid))
            continue;
        q = quota_create();
        if (getquota(conf, fsname, sb.st_uid, q) != 0) {
            quota_destroy(q);
            continue; 
        }
        list_append(qlist, q);
    }
    if (closedir(dir) < 0)
        fprintf(stderr, "%s: closedir %s: %m\n", prog, conf->cf_path);
}

static void
pwscan(confent_t *conf, List qlist, uid_t minuid, uid_t maxuid, char *fsname)
{
    struct passwd *pw;
    quota_t *q;

    while ((pw = getpwent()) != NULL) {
        if (minuid && pw->pw_uid < minuid)
            continue;
        if (maxuid != -1 && pw->pw_uid > maxuid)
            continue;
        if (list_find_first(qlist, (ListFindF)quota_match_uid, &pw->pw_uid))
            continue;
        q = quota_create();
        if (getquota(conf, fsname, pw->pw_uid, q) != 0) {
            quota_destroy(q);
            continue;
        }
        list_append(qlist, q);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
