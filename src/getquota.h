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

typedef struct quota_struct *quota_t;

quota_t quota_create(char *label, char *rhost, char *rpath, int thresh);
void quota_destroy(quota_t q);

int quota_get(uid_t uid, quota_t q);
void quota_adduser(quota_t q, char *name);

int quota_match_uid(quota_t x, uid_t *key);
int quota_cmp_uid(quota_t x, quota_t y);
int quota_cmp_uid_reverse(quota_t x, quota_t y);
int quota_cmp_bytes(quota_t x, quota_t y);
int quota_cmp_bytes_reverse(quota_t x, quota_t y);
int quota_cmp_files(quota_t x, quota_t y);
int quota_cmp_files_reverse(quota_t x, quota_t y);

void quota_report_heading(void);
void quota_report_heading_usageonly(void);
int quota_report(quota_t x, unsigned long *bsize);
int quota_report_usageonly(quota_t x, unsigned long *bsize);
int quota_report_h(quota_t x, unsigned long *bsize);
int quota_report_usageonly_h(quota_t x, unsigned long *bsize);

void quota_print_heading(char *name);
int quota_print(quota_t x, void *arg);
int quota_print_realpath(quota_t x, void *arg);
int quota_print_justwarn(quota_t x, int *msgcount);
int quota_print_justwarn_realpath(quota_t x, int *msgcount);

int quota_print_raw(quota_t q, void *arg);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

