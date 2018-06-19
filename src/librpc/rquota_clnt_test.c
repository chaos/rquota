/*****************************************************************************
 *  Copyright (C) 2001-2018 The Regents of the University of California.
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
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <getopt.h>

#include "rquota.h"

static const char *prog = "rquota_clnt_test";

void die (const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf, sizeof (buf), fmt, ap);
    va_end (ap);

    fprintf (stderr, "%s: %s\n", prog, buf);
    exit (1);
}

void usage (void)
{
    fprintf (stderr, "Usage: %s hostname filesystem\n", prog);
    exit (1);
}

void print_quota (struct rquota *rq)
{
    printf("rq_bsize=%llu rq_curblocks=%llu rq_bsoftlimit=%llu "
           "rq_bhardlimit=%llu rq_btimeleft=%llu rq_curfiles=%llu "
           "rq_fsoftlimit=%llu rq_fhardlimit=%llu rq_ftimeleft=%llu\n",
           (unsigned long long)rq->rq_bsize,
           (unsigned long long)rq->rq_curblocks,
           (unsigned long long)rq->rq_bsoftlimit,
           (unsigned long long)rq->rq_bhardlimit,
           (unsigned long long)rq->rq_btimeleft,
           (unsigned long long)rq->rq_curfiles,
           (unsigned long long)rq->rq_fsoftlimit,
           (unsigned long long)rq->rq_fhardlimit,
           (unsigned long long)rq->rq_ftimeleft);
}

void tv_double (double t, struct timeval *tv)
{
    tv->tv_sec = t;
    tv->tv_usec = (t - tv->tv_sec)*1E6;
}

#define OPTIONS "N:R:"
static const struct option longopts[] = {
    {"nfs-timeout",      required_argument,  0, 'N'},
    {"nfs-retry-timeout",required_argument,  0, 'R'},
    {0, 0, 0, 0},
};

int main(int argc, char **argv)
{
    uid_t uid = geteuid ();
    uid_t gid = getegid ();
    CLIENT *cl;
    getquota_rslt *result;
    getquota_args args;
    char localhost[MAXHOSTNAMELEN + 1];
    char *host;
    char *filesystem;
    double retry_timeout = -1.;
    double total_timeout = -1.;
    int c;

    while ((c = getopt_long (argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'R':   // --nfs-retry-timeout=SEC
                retry_timeout = strtod (optarg, NULL);
                break;
            case 'N':   // --nfs-timeout=SEC
                total_timeout = strtod (optarg, NULL);
                break;
            default:
                usage ();
        }
    };
    if (optind != argc - 2)
        usage ();
    host = argv[optind++];
    filesystem = argv[optind++];

    if (!(cl = clnt_create (host, RQUOTAPROG, RQUOTAVERS, "udp")))
        die ("clnt_create");
    if (retry_timeout >= 0.) {
        struct timeval tv;
        tv_double (retry_timeout, &tv);
        if (!clnt_control (cl, CLSET_RETRY_TIMEOUT, (char *)&tv))
            die ("clnt_control CLSET_RETRY_TIMEOUT");
    }
    if (total_timeout >= 0.) {
        struct timeval tv;
        tv_double (total_timeout, &tv);
        if (!clnt_control (cl, CLSET_TIMEOUT, (char *)&tv))
            die ("clnt_control CLSET_TIMEOUT");
    }
    if (gethostname (localhost, sizeof (localhost)) < 0)
        die ("gethostname");
    if (!(cl->cl_auth = authunix_create (localhost, uid, gid, 0, NULL)))
        die ("authunix_create");
    args.gqa_pathp = filesystem;
    args.gqa_uid = uid;
    if (!(result = rquotaproc_getquota_1 (&args, cl)))
        die ("%s", clnt_sperror (cl, host));
    if (result->gqr_status == Q_NOQUOTA)
        die ("No quota");
    if (result->gqr_status == Q_EPERM)
        die ("Permission denied");
    if (result->gqr_status != Q_OK)
        die ("Error %d", result->gqr_status);
    print_quota (&result->getquota_rslt_u.gqr_rquota);
    auth_destroy (cl->cl_auth);
    clnt_destroy (cl);
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

