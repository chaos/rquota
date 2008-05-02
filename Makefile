PROJECT=quota
CFFILE=	quota.fs
RSRC=	rquota_xdr.c rquota_clnt.c rquota.h
ROBJS=	rquota_xdr.o rquota_clnt.o
OBJS= 	$(ROBJS) getconf.o util.o getquota_nfs.o getquota_lustre.o

# uncomment for Solaris
#LIBADD=	-lnsl -lsocket
CC=	gcc
CFLAGS=	-Wall
LIBS=	-lrpcsvc -llustreapi $(LIBADD)

all:	quota lquota nquota 

quota: quota.c $(OBJS) rquota.h
	$(CC) $(CFLAGS) -o $@ quota.c $(OBJS) $(LIBS)

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

lquota: getquota_lustre.c getquota.h util.o
	$(CC) $(CFLAGS) -DSTAND -o $@ getquota_lustre.c util.o $(LIBS)

nquota: getquota_nfs.c getquota.h util.o $(ROBJS)
	$(CC) $(CFLAGS) -DSTAND -o $@ getquota_nfs.c $(ROBJS) util.o $(LIBS)

clean:
	rm -f quota lquota nquota 
	rm -f *.o
	rm -f $(RSRC) 
