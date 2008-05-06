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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Convert integer size to string.
 */
char *
size2str(unsigned long long size, char *str, int len)
{
    float n = (float)size / 1024; /* kilobytes */

    /* Note: try to display values above 1000 as the next unit,
     * i.e. 1000-1023MB should be displayed as GB.  Values should never
     * take up more chars than "999.9G".
     */
    if (n >= 1000*1024*1024)
        snprintf(str, len, "%.1fT", n / (1024*1024*1024));
    else if (n >= 1000*1024)
        snprintf(str, len, "%.1fG", n / (1024*1024));
    else if (n >= 1000)
        snprintf(str, len, "%.1fM", n / 1024);
    else if (n > 0)
        snprintf(str, len, "%.1fK", n);
    else
        snprintf(str, len, "-0-");
    return str;
}

char *
xstrdup(char *str)
{
    char *cpy = strdup(str);

    if (!cpy) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return cpy;
}

void *
xmalloc(size_t size)
{
    void *ptr = malloc(size);
    
    if (!ptr) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return ptr;
}

/* Match a directory against a mountpoint containing it.
 * We must match whole path components, see
 *  https://chaos.llnl.gov/bugzilla/show_bug.cgi?id=301
 */
int
match_path(char *dir, const char *mountpoint)
{
    int n = strlen(mountpoint);
    int m = strlen(dir);

    if (!strcmp(mountpoint, "/") || !strcmp(mountpoint, dir))
        return 1;
    if (n > m)
	return 0;
    if (!strncmp(dir, mountpoint, n) && dir[n] == '/')
	return 1;
    return 0;
}

void
test_match_path(void)
{
    char s[256];

    assert(!match_path(strcpy(s, "/g/g53/foo"), "/g/g5"));
    assert(match_path(strcpy(s, "/g/g53/foo"), "/g/g53"));
    assert(!match_path(strcpy(s, "/g/g5/foo"), "/g/g53"));
    assert(match_path(strcpy(s, "/g/g5/foo"), "/g/g5"));

    assert(match_path(strcpy(s, "/g/g53/a/b/c/d/e/f/g/h/i/j"), "/g"));
    assert(match_path(strcpy(s, "/g/g53//a/b/c/d/e/f/g/h/i/j"), "/g/g53"));

    assert(match_path(strcpy(s, "/home/foo"), "/home"));
    assert(match_path(strcpy(s, "/home/foo"), "/home/foo"));

    assert(match_path(strcpy(s, "/a"), "/"));
    assert(match_path(strcpy(s, "/home/foo"), "/"));
}

unsigned long
parse_blocksize(char *s, unsigned long *b)
{
    int err = 0;
    char *end;

    *b = strtoul(s, &end, 10);
    switch (end[0]) {
        case '\0':
        case 'b':
        case 'B':
            break;
        case 'k':
        case 'K':
            *b *= 1024;
            break;
        case 'm':
        case 'M':
            *b *= (1024*1024);
            break;
        case 'g':
        case 'G':
            *b *= (1024*1024*1024);
            break;
        default:
            err++;
            break;
    }
    if (end[0] && end[1])
        err++;
    return err;
}

/*
 * lsd_* functions are needed by list.[ch].
 */

void
lsd_fatal_error(char *file, int line, char *mesg)
{
    fprintf(stderr, "fatal error: %s: %s::%d: %s",
                    mesg, file, line, strerror(errno));
    exit(1);
}

void *
lsd_nomem_error(char *file, int line, char *mesg)
{
    fprintf(stderr, "out of memory: %s: %s::%d", mesg, file, line);
    exit(1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
