/*
 * $Id$
 *
 * Copyright (C) 1995-2000  Jim Garlick
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include "getconf.h"

static char *
next_field(p, sep)
	char **p;
	char sep;
{
        char *rv = *p;

        while (**p != '\0' && **p != sep && **p != '\n')
                (*p)++;
        *(*p)++ = '\0';

        return(rv);
}

static FILE *fsys = NULL;

void
setconf_ent(char *path)
{
	if (!fsys) {
		fsys = fopen(path, "r");
		if (fsys == NULL) {
			perror(path);
			exit(1);
		}
	} else
		rewind(fsys);
}

void
endconf_ent()
{
	if (fsys)
		fclose(fsys);
}

sysfs_t *
getconf_ent()
{
	static char buf[BUFSIZ];
	static sysfs_t rv;
	char *p;

	if (!fsys)
		setconf_ent(_PATH_QUOTA_CONF);

	if (fsys && fgets(buf, BUFSIZ, fsys)) {
		p = buf;
		rv.desc = next_field(&p, ':');
		rv.host = next_field(&p, ':');
		rv.path = next_field(&p, ':');
		rv.thresh = atoi(next_field(&p, ':'));
		return &rv;
	}

	return NULL;
}
