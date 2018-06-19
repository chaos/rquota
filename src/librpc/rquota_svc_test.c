/*  Copyright (C) 2001-2018 The Regents of the University of California.
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
#include <unistd.h>
#include "rquota.h"

/* TODO: Server implementation for testing
 */

void rquota_bump (struct rquota *rq)
{
    rq->rq_bsize++;
    rq->rq_active++;

    rq->rq_bhardlimit++;
    rq->rq_bsoftlimit++;
    rq->rq_curblocks++;

    rq->rq_fhardlimit++;
    rq->rq_fsoftlimit++;
    rq->rq_curfiles++;

    rq->rq_btimeleft++;
    rq->rq_ftimeleft++;
}

getquota_rslt *rquotaproc_getquota_1_svc(getquota_args *args,
					 struct svc_req *req)
{
    static getquota_rslt res;

    fprintf (stderr, "%s: uid=%d path=%s\n", __FUNCTION__,
             args->gqa_uid, args->gqa_pathp);
    if (!(strcmp (args->gqa_pathp, "drop")))
        return NULL; // no response
    if (!(strcmp (args->gqa_pathp, "exit")))
        exit (0);
    res.gqr_status = Q_OK;
    rquota_bump (&res.getquota_rslt_u.gqr_rquota);
    return &res;
}

getquota_rslt * rquotaproc_getactivequota_1_svc(getquota_args *args,
						struct svc_req *req)
{
    static getquota_rslt res;

    fprintf (stderr, "%s: uid=%d path=%s\n", __FUNCTION__,
             args->gqa_uid, args->gqa_pathp);
    res.gqr_status = Q_OK;
    rquota_bump (&res.getquota_rslt_u.gqr_rquota);
    return &res;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

