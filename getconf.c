/*
 * $Id$
 *
 * Copyright (C) 1995-2002  Jim Garlick
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

/* configuration file pointer */
static FILE *fsys = NULL;

/*
 * Helper for getconfent().  Parse out a field within a string.
 * Replace separators with NULL's and return the field or NULL if at end.
 * 	p (IN/OUT)	string pointer - moved past next separator
 * 	sep (IN)	separator character
 *	RETURN		the base string (now \0 terminated at sep position)
 */
static char *
next_field(char **str, char sep)
{
        char *rv = *str;

        while (**str != '\0' && **str != sep && **str != '\n')
                (*str)++;
        *(*str)++ = '\0';

        return rv;
}

/*
 * Open/rewind the config file
 * 	path (IN)	pathname to open if not open already
 */
void
setconfent(char *path)
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

/*
 * Close the config file if open.
 */
void
endconfent(void)
{
	if (fsys)
		fclose(fsys);
}

/*
 * Return the next configuration file entry.  Open the default config file 
 * path if the config file has not already been opened.
 * 	RETURN		config file entry
 */
confent_t *
getconfent(void)
{
	static char buf[BUFSIZ];
	static confent_t conf;
	confent_t *result = NULL;

	if (!fsys)
		setconfent(_PATH_QUOTA_CONF);

	if (fgets(buf, BUFSIZ, fsys)) {
		char *threshp;
		char *p = buf;

		conf.cf_desc = next_field(&p, ':');
		conf.cf_host = next_field(&p, ':');
		conf.cf_path = next_field(&p, ':');
		threshp = next_field(&p, ':');
		conf.cf_thresh = 0;
		if (threshp != NULL)
			conf.cf_thresh = atoi(threshp);

		result = &conf;
	}

	return result;
}
