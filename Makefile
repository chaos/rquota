CC=	gcc
CFLAGS=	-Wall -DWITH_LUSTRE=1 -DWITH_NFS=1
OBJS=	getquota_nfs.o rquota_xdr.o rquota_clnt.o getquota_lustre.o \
	getconf.o util.o list.o getquota.o
# uncomment for Solaris
#LIBADD= -lnsl -lsocket

all: repquota quota

quota: quota.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ quota.o $(OBJS) -lrpcsvc -llustreapi $(LDADD)
repquota: repquota.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ repquota.o $(OBJS) -lrpcsvc -llustreapi $(LDADD)

getquota_lustre.o quota.o: getquota.h
getquota_nfs.o: rquota.h
rquota.h: rquota.x
	rpcgen -o $@ -h rquota.x
rquota_xdr.c: rquota.x rquota.h
	rpcgen -o $@ -c rquota.x
rquota_clnt.c: rquota.x rquota.h
	rpcgen -o $@ -l rquota.x
rquota_xdr.o: rquota_xdr.c
	$(CC) $(CFLAGS) -Wno-unused -o $@ -c $<
rquota_clnt.o: rquota_clnt.c
	$(CC) $(CFLAGS) -Wno-unused -o $@ -c $<

clean:
	rm -f quota repquota
	rm -f lquota nquota 
	rm -f rquota_xdr.c rquota_clnt.c rquota.h
	rm -f *.o
