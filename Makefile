PROJECT=quota
CFFILE=	quota.fs
OBJS= 	quota_clnt.o quota_xdr.o quota.o getconf.o
CLEAN=	quota_xdr.c quota_clnt.c quota.h

CC=	gcc
CFLAGS=	-Wall
LIBS=	-lrpcsvc $(LIBADD)
# uncomment for Solaris
#LIBADD=	-lnsl -lsocket

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
