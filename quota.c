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
#include <sys/param.h> /* MAXHOSTNAMELEN */
#include "getconf.h"
#include "quota.h"

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
abbrev(float kval, char *str, int len)
{
	/* 
	 * Note: try to display values above 1000 as the next unit,
	 * i.e. 1000-1023MB should be displayed as GB.  Values should never
	 * take up more chars than "999.9G".
	 */
	if (kval >= 1000*1024*1024) {
                snprintf(str, len, "%.1fT", kval / (1024*1024*1024));
        } else if (kval >= 1000*1024) {
                snprintf(str, len, "%.1fG", kval / (1024*1024));
        } else if (kval >= 1000) {
                snprintf(str, len, "%.1fM", kval / 1024);
        } else if (kval > 0) {
                snprintf(str, len, "%.1fK", kval);
	} else /* if (kval == 0) */ {
                snprintf(str, len, "-0-");
	}
	str[len - 1] = '\0'; /* truncation is not fatal, ensure termination */

        return str;
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
	char myhost[MAXHOSTNAMELEN];

	/* set up getquota_args */
	args.gqa_pathp = strdup(path);
	args.gqa_uid = uid;

	/* create client handle, adding AUTH_UNIX */
	cl = clnt_create(system, RQUOTAPROG, RQUOTAVERS, "udp");
	if (cl == NULL)
		return RV_CLNTCREATERR;

#if 0
	/* GNATS #48 - fails if in >16 groups on Tru64 */
	cl->cl_auth = authunix_create_default();
#else
	if (gethostname(myhost, MAXHOSTNAMELEN) != 0)
		return RV_CLNTCREATERR;
	cl->cl_auth = authunix_create(myhost, uid, getgid(), 0, NULL);
#endif

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
display_header(char *name)
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
	char bdays[TMPSTRSZ], fdays[TMPSTRSZ], tmpstr[TMPSTRSZ];

	/* build bdays[] */
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
	} else {
		bdays[0] = '\0';
	}

	/* build fdays[] */
	if (HASFILE(rq) && OVERFILE(rq)) {
		if (NSTARTFILE(rq)) {
			sprintf(fdays, "[7 days]");
		} else {
			if (EXPFILE(rq))
				sprintf(fdays, "expired");
			else	/* if (rq.rq_ftimeleft > 0) */
				sprintf(fdays, "%.1f day%s",
				    DAYSFILE(rq),
				    DAYSFILE(rq) > 1.0 ? "s" : "");
		}
	} else {
		fdays[0] = '\0';
	}

	/* filesystem name */
	printf("%-15s", fsname);
	if (strlen(fsname) > 14)
		printf("\n%-15s", ""); /* make future columns line up */

	/* block used/quota/limit values */
	printf("%-7s", abbrev(rq.rq_curblocks * SCALE(rq), tmpstr, TMPSTRSZ));
	if (HASBLOCK(rq)) {
		char softblock[TMPSTRSZ], hardblock[TMPSTRSZ];

		abbrev(rq.rq_bsoftlimit * SCALE(rq), softblock, TMPSTRSZ);
		abbrev(rq.rq_bhardlimit * SCALE(rq), hardblock, TMPSTRSZ);

		printf("%-7s%-9s%-10s", softblock, hardblock, bdays);
	} else {
		printf("%-7s%-9s%-10s", "n/a", "n/a", "");
	}

	/* file used/quota/limit values */
	printf("%-7s", abbrev(rq.rq_curfiles / 1024.0, tmpstr, TMPSTRSZ));
	if (HASFILE(rq)) {
		char softfile[TMPSTRSZ], hardfile[TMPSTRSZ];

		abbrev(rq.rq_fsoftlimit / 1024.0, softfile, TMPSTRSZ);
		abbrev(rq.rq_fhardlimit / 1024.0, hardfile, TMPSTRSZ);

		printf("%-7s%-9s%s\n", softfile, hardfile, fdays);
	} else {
		printf("%-7s%-9s\n", "n/a", "n/a");
	}

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
	char tmpstr[TMPSTRSZ];

	if (HASBLOCK(rq) && OVERBLOCK(rq)) {
		printf("Over disk quota on %s, ", fsname);
		if (NSTARTBLOCK(rq))
			printf("remove %s within [7 days]\n",
			    abbrev(REMOVEBLOCKS(rq) * SCALE(rq), 
				    tmpstr, TMPSTRSZ));
		else
			if (EXPBLOCK(rq))
				printf("time limit expired (run quota -v)\n");
			else
				printf("remove %s within %.1f days\n",
				    abbrev(REMOVEBLOCKS(rq) * SCALE(rq), 
					    tmpstr, TMPSTRSZ),
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
 *	conf (IN)	file system to get quota for
 *	pw (IN)		password entry for user to get quota for
 */
static void 
report(int ropt, int vopt, confent_t *conf, struct passwd *pw)
{
	char    tmp[TMPSTRSZ];
	int     rv;
	struct rquota rq;
	char   *desc;

	if (ropt) {
		sprintf(tmp, "%s:%s", conf->cf_host, conf->cf_path);
		desc = tmp;
	} else
		desc = conf->cf_desc;
	rv = getquota(pw->pw_uid, conf->cf_host, conf->cf_path, &rq);
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
			long_report(desc, rq, conf->cf_thresh);
		else
			short_report(desc, rq, conf->cf_thresh);
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
	confent_t *conf;
	int	c;
	extern char *optarg;
	extern int optind;

	now = time(0);

	while ((c = getopt(argc, argv, "f:rv")) != EOF) {
		switch(c) {
			case 'r':	/* display remote mount pt. */
				ropt = 1;
				break;
			case 'v':	/* verbose output */
				vopt = 1;
				break;
			case 'f':	/* alternate config file */
				setconfent(optarg);
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

	/* 
	 * Start the report. 
	 */

	/* stderr -> stdout */
	close(2);		
	dup(1);

	/* display initial header */
	if (vopt)		
		display_header(pw->pw_name);

	/* report on each configured filesystem */
	while ((conf = getconfent()) != NULL) {
		report(ropt, vopt, conf, pw);
	}

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: quota [-v] [-r] [user]\n");
	exit(1);
}
