# $Id$

PROJECT=quota
CFFILE=	quota.fs
OBJS= 	quota_clnt.o quota_xdr.o quota.o getconf.o
CLEAN=	quota_xdr.c quota_clnt.c quota.h

# UNICOS
#CC=    /opt/ctl/bin/cc
#CFLAGS=
# everything else
CC=	gcc
CFLAGS=	-Wall

# AIX, DEC/OSF, HP-UX, IRIX
LIBS=	-lrpcsvc
# Solaris
#LIBS=	-lrpcsvc -lnsl -lsocket

all:	quota

quota: $(OBJS) quota.h
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

quota.h: quota.x
	rpcgen -o quota.h -h quota.x

quota_xdr.c: quota.x quota.h
	rpcgen -o quota_xdr.c -c quota.x

quota_clnt.c: quota.x quota.h
	rpcgen -o quota_clnt.c -l quota.x

quota_svc.c:  quota.x quota.h
	rpcgen -o quota_svc.c -m quota.x

clean:
	rm -f $(OBJS) quota $(CLEAN)

include Make-rpm.mk
