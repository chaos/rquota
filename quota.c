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

 /*
 * Mystery: what is rq_active?
 */
#include <ctype.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "getconf.h"
#include "quota.h"

typedef enum { false, true } bool;

#define TMPSTRSZ        1024

/* timeleft > 1 year days means "not started" (0 - now) */
#define MAX_TIMELEFT    (60*60*24*365)

/*
 * Return values for getquota().
 */
#define RV_OK           0
#define RV_EPERM        1
#define RV_NOQUOTA      2
#define RV_CLNTCREATERR 3
#define RV_CLNTCALLERR  4
#define RV_BADRETVAL    5

/*
 * Report header for -v output.
 */
#define HDR1 "Filesystem     "
#define HDR2 "used   quota  limit    timeleft  "
#define HDR3 "files  quota  limit    timeleft"

/*
 * Macros for manipulating struct rquota.
 */
#define SCALE(x)        (x.rq_bsize / 1024.0)
#define DAYSBLOCK(x)    ((float)x.rq_btimeleft / (60*60*24))
#define DAYSFILE(x)     ((float)x.rq_ftimeleft / (60*60*24))
#define OVERBLOCK(x)    (x.rq_curblocks >= x.rq_bsoftlimit)
#define OVERFILE(x)     (x.rq_curfiles >= x.rq_fsoftlimit)
#define REMOVEBLOCKS(x) (x.rq_curblocks - x.rq_bsoftlimit + 1)
#define REMOVEFILES(x)  (x.rq_curfiles - x.rq_fsoftlimit + 1)
#define EXPFILE(x)      ((int)(x.rq_ftimeleft) < 0)
#define EXPBLOCK(x)     ((int)(x.rq_btimeleft) < 0)

/* 
 * Normally 0 indicates no quota, but Network Appliance servers return -1
 * (in a 32-bit unsigned long!  BAD NetApp!).  Since Cray's don't have a 
 * 32-bit integer type, we can't express this value as (u_long)(-1), hence
 * the test for 0xffffffff.
 */
#define QVALID(x)	((x) != 0 && (x) != 0xffffffff)
#define HASBLOCK(x)     (QVALID(x.rq_bhardlimit) && QVALID(x.rq_bsoftlimit))
#define HASFILE(x)      (QVALID(x.rq_fhardlimit) && QVALID(x.rq_fsoftlimit))

#define HASTHRESH(t)	(t > 0)
#define OVERTHRESH(x,t) (x.rq_curblocks >= (x.rq_bhardlimit * (t/100.0)))

/*
 * Rpc.rquotad simply subtracts the current time from the timeout value
 * and returns it to us in the timeleft elements.  Therefore, EXPIRED is
 * a negative value of timeleft, but NOT STARTED is also a negative value
 * approaching (0 - now), e.g. a timeout around Jan 1, 1970.  Because the
 * rpc call has latency and the clocks may not be synchronized, we throw
 * MAX_SKEW into the equation.
 */
#define MAX_SKEW      (60*60*24)
#define NSTARTFILE(x) (EXPFILE(x) && abs((int)(x.rq_ftimeleft)+now)<=MAX_SKEW)
#define NSTARTBLOCK(x)(EXPBLOCK(x) && abs((int)(x.rq_btimeleft)+now)<=MAX_SKEW)
CLIENT *cl;
static time_t now;

static void usage(void);

/*
 * Abbreviate numerical sizes provided in K (2^10).
 *	kval (IN)      value in K bytes
 * 	ret (RETURN)   abbreviated value in M, G, or T
 */
static char *
abbrev(float kval)
{
	static char ret[TMPSTRSZ];

	/* 
	 * Note: try to display values above 1000 as the next unit,
	 * i.e. 1000-1023MB should be displayed as GB.  Values should never
	 * take up more chars than "999.9G".
	 */
	if (kval >= 1000*1024*1024) {
                sprintf(ret, "%.1fT", kval / (1024*1024*1024));
        } else if (kval >= 1000*1024) {
                sprintf(ret, "%.1fG", kval / (1024*1024));
        } else if (kval >= 1000) {
                sprintf(ret, "%.1fM", kval / 1024);
        } else if (kval > 0) {
                sprintf(ret, "%.1fK", kval);
	} else /* if (kval == 0) */ {
                sprintf(ret, "-0-");
	}

        return ret;
}

/*
 * Look up quotas on remote system and return them in rq structure.
 * Function return value is one of the RV_* values above.
 * 	uid (IN)		uid to query
 * 	system (IN)		name of server
 * 	path (IN)		path on server
 * 	rq (OUT) 		quota information
 */
static int 
getquota(uid_t uid, char *system, char *path, struct rquota * rq)
{
	static getquota_args args;
	getquota_rslt *result;

	/* set up getquota_args */
	args.gqa_pathp = strdup(path);
	args.gqa_uid = uid;

	/* create client handle, adding AUTH_UNIX */
	cl = clnt_create(system, RQUOTAPROG, RQUOTAVERS, "udp");
	if (cl == NULL)
		return RV_CLNTCREATERR;
	cl->cl_auth = authunix_create_default();

	/* call remote procedure */
	result = rquotaproc_getquota_1(&args, cl);
	if (result == NULL)
		return RV_CLNTCALLERR;

	switch (result->gqr_status) {
	case Q_NOQUOTA:
		return RV_NOQUOTA;
	case Q_EPERM:
		return RV_EPERM;
	case Q_OK:
		memcpy(rq,
		    &(result->getquota_rslt_u), sizeof(struct rquota));
		return RV_OK;
	}
	return RV_BADRETVAL;
}

/* 
 * Display header.
 * 	name (IN) 	name of user
 */
static void 
hdr(char *name)
{
	printf("Disk quotas for %s:\n", name);
	printf("%s%s%s\n", HDR1, HDR2, HDR3);
}

/*
 * Long report.
 * 	fsname (IN)	filesystem name
 * 	rq (IN)		quota information
 * 	thresh (IN)	% usage after which user should be warned
 */
static void 
long_report(char *fsname, struct rquota rq, int thresh)
{
	char bdays[TMPSTRSZ], fdays[TMPSTRSZ];
	char *hardblock, *softblock;

	if (HASBLOCK(rq) && OVERBLOCK(rq)) {
		if (NSTARTBLOCK(rq))
			sprintf(bdays, "[7 days]");
		else
			if (EXPBLOCK(rq))
				sprintf(bdays, "expired");
			else
				sprintf(bdays, "%.1f day%s",
				    DAYSBLOCK(rq),
				    DAYSBLOCK(rq) > 1.0 ? "s" : "");
	} else
		bdays[0] = '\0';

	if (HASFILE(rq) && OVERFILE(rq)) {
		if (NSTARTFILE(rq))
			sprintf(fdays, "[7 days]");
		else
			if (EXPFILE(rq))
				sprintf(fdays, "expired");
			else	/* if (rq.rq_ftimeleft > 0) */
				sprintf(fdays, "%.1f day%s",
				    DAYSFILE(rq),
				    DAYSFILE(rq) > 1.0 ? "s" : "");
	} else
		fdays[0] = '\0';

	printf("%-15s", fsname);
	if (strlen(fsname) > 14)
		printf("\n%-15s", "");
	printf("%-7s", abbrev((rq.rq_curblocks * SCALE(rq))));


	if (HASBLOCK(rq)) {
		softblock = strdup(abbrev(rq.rq_bsoftlimit * SCALE(rq)));
		hardblock = strdup(abbrev(rq.rq_bhardlimit * SCALE(rq)));

		printf("%-7s%-9s%-10s", softblock, hardblock, bdays);

		free(hardblock); free(softblock);
	} else
		printf("%-7s%-9s%-10s", "n/a", "n/a", "");

	printf("%-7d", (int) rq.rq_curfiles);

	if (HASFILE(rq))
		printf("%-7d%-9d%s\n", (int) rq.rq_fsoftlimit, 
		    (int) rq.rq_fhardlimit, fdays);
	else
		printf("%-7s%-9s\n", "n/a", "n/a");

	/* tack on a line if over percentage specified in quota.fs */
	if (HASBLOCK(rq) && HASTHRESH(thresh) && OVERTHRESH(rq, thresh))
		printf("***  usage on %s has exceeded %d%% of quota!\n", 
		    fsname, thresh);
}

/*
 * Short report.
 * 	fsname (IN)	filesystem name
 * 	rq (IN)		quota information
 * 	thresh (IN)	% usage after which user should be warned
 */
static void 
short_report(char *fsname, struct rquota rq, int thresh)
{
	if (HASBLOCK(rq) && OVERBLOCK(rq)) {
		printf("Over disk quota on %s, ", fsname);
		if (NSTARTBLOCK(rq))
			printf("remove %s within [7 days]\n",
			    abbrev((REMOVEBLOCKS(rq) * SCALE(rq))));
		else
			if (EXPBLOCK(rq))
				printf("time limit expired (run quota -v)\n");
			else
				printf("remove %s within %.1f days\n",
				    abbrev((REMOVEBLOCKS(rq) * SCALE(rq))),
				    DAYSBLOCK(rq));
	} else if (HASBLOCK(rq) 
	    && HASTHRESH(thresh) && OVERTHRESH(rq, thresh)) {
		printf("Warning: usage on %s has exceeded %d%% of quota.\n", 
		    fsname, thresh);
		printf("Run quota -v for more detailed information.\n");
	}	

	if (HASFILE(rq) && OVERFILE(rq)) {
		printf("Over file quota on %s, ", fsname);
		if (NSTARTFILE(rq))
			printf("remove %d files within [7 days]\n",
			    (int) REMOVEFILES(rq));
		else
			if (EXPFILE(rq))
				printf("time limit expired (run quota -v)\n");
			else
				printf("remove %d files within %.1f days\n",
				    (int) REMOVEFILES(rq),
				    DAYSFILE(rq));
	}
}

/*
 * Decode/print error number 
 *	fsname (IN)	filesystem name
 * 	rv (IN) 	return code from getquota()
 */
static void 
display_error(char *fsname, int rv)
{
	char    tmp[TMPSTRSZ];

	switch (rv) {
	case RV_NOQUOTA:
		printf("%-14s no quota\n", fsname);
		break;
	case RV_EPERM:
		printf("%-14s permission denied\n", fsname);
		break;
	case RV_CLNTCREATERR:
		sprintf(tmp, "%s", fsname);
		clnt_pcreateerror(tmp);
		break;
	case RV_CLNTCALLERR:
		sprintf(tmp, "%s", fsname);
		clnt_perror(cl, tmp);
		break;
	case RV_BADRETVAL:
		printf("%-14s bad RPC return value\n", fsname);
		break;
	default:
		printf("%-14s unknown return value: %d\n", fsname, rv);
		break;
	}
}

/*
 * Display data about a filesystem.
 *	ropt (IN)	show real filesystem names rather than descriptive ones
 *	vopt (IN)	verbose mode
 *	sfs (IN)	file system to get quota for
 *	pw (IN)		password entry for user to get quota for
 */
static void 
doit(int ropt, int vopt, sysfs_t * sfs, struct passwd * pw)
{
	char    tmp[TMPSTRSZ];
	int     rv;
	struct rquota rq;
	char   *desc;

	if (ropt) {
		sprintf(tmp, "%s:%s", sfs->host, sfs->path);
		desc = tmp;
	} else
		desc = sfs->desc;
	rv = getquota(pw->pw_uid, sfs->host, sfs->path, &rq);
#ifdef DEC_HACK
	/*
	 * XXX Hack for DEC NFS servers running OSF/1.  The DEC RPC quota 
	 * service returns 2-block limits on filesystems in some cases where 
	 * the user has a zero limit in the quota file.  Turn these in to 
	 * RV_NOQUOTA so we don't see them in our output.
	 */
	if (rv == RV_OK && rq.rq_bhardlimit == 2 && rq.rq_bsoftlimit == 2)
		rv = RV_NOQUOTA;
#endif
	if (rv == RV_OK) {
		if (vopt)
			long_report(desc, rq, sfs->thresh);
		else
			short_report(desc, rq, sfs->thresh);
	} else
		if (vopt && rv != RV_NOQUOTA)
			display_error(desc, rv);
}

int 
main(int argc, char *argv[])
{
	int    vopt = 0, ropt = 0;
	char   *user = NULL;
	struct passwd *pw;
	sysfs_t	*sfs;
	int	c;
	extern char *optarg;
	extern int optind;

	now = time(0);

	while ((c = getopt(argc, argv, "rv")) != EOF) {
		switch(c) {
			case 'r':
				ropt = 1;
				break;
			case 'v':
				vopt = 1;
				break;
			default:
				usage();
		}
	}
	if (optind < argc)
		user = argv[optind++];
	if (optind < argc)
		usage();

	/* getlogin() not appropriate here */
	if (!user)
		pw = getpwuid(getuid());
	else {
		if (isdigit(*user))
			pw = getpwuid(atoi(user));
		else
			pw = getpwnam(user);
	}
	if (!pw) {
		fprintf(stderr, "quota: no such user%s%s\n",
		    user ? ": " : "",
		    user ? user : "");
		exit(1);
	}

	/* Start the report. */
	close(2);		/* stderr -> stdout */
	dup(1);
	if (vopt)		/* display initial header */
		hdr(pw->pw_name);
	while ((sfs = getconf_ent()) != NULL)
		doit(ropt, vopt, sfs, pw);

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: quota [-v] [-r] [user]\n");
	exit(1);
}
