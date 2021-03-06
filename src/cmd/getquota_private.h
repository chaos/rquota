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

typedef enum { NONE, UNDER, NOTSTARTED, STARTED, EXPIRED } qstate_t;
/*
 * NONE     quotas not set
 * UNDER    used < soft
 * NOTSTARTED  soft < used < hard && no grace period set - NFS only
 * STARTED  soft < used < hard && now < grace_expiration
 * EXPIRED  hard < used || (soft < used < hard && now >= grace_expiration)
 */

#define QUOTA_MAGIC 0x3434aaaf
struct quota_struct {
    int                q_magic;
    uid_t              q_uid;
    char              *q_name;
    char              *q_label;        /* assumed to be local mount point */
    char              *q_rhost;        /* lustre: set to "lustre" */
    char              *q_rpath;        /* lustre: set to local mount pt */
    int                q_thresh;       /* 0 = unused */
    unsigned long long q_bytes_used;
    unsigned long long q_bytes_softlim;/* 0 = no limit */
    unsigned long long q_bytes_hardlim;/* 0 = no limit */
    unsigned long long q_bytes_secleft;/* valid if state == STARTED */
    qstate_t           q_bytes_state;

    unsigned long long q_files_used;
    unsigned long long q_files_softlim;/* 0 = no limit */
    unsigned long long q_files_hardlim;/* 0 = no limit */
    unsigned long long q_files_secleft;/* valid if state == STARTED */
    qstate_t           q_files_state;
};

int quota_get_lustre(uid_t uid, quota_t q);
int quota_get_nfs(uid_t uid, quota_t q);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
