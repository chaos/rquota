/*
 * $Id$
 *
 * Copyright (C) 1995-2000  Jim Garlick
 */
typedef struct {
	char *desc;
	char *host;
	char *path;
	int thresh;
} sysfs_t;	

#ifndef _PATH_QUOTA_CONF
#define _PATH_QUOTA_CONF "/etc/quota.conf"
#endif

void setconf_ent();
void endconf_ent();
sysfs_t *getconf_ent();
