AM_CFLAGS = -Wall -Wno-unused

bin_PROGRAMS = quota repquota
quota_SOURCES = quota.c \
	getquota_nfs.c rquota_xdr.c rquota_clnt.c getquota_lustre.c \
	getconf.c util.c list.c getquota.c
repquota_SOURCES = repquota.c \
	getquota_nfs.c rquota_xdr.c rquota_clnt.c getquota_lustre.c \
	getconf.c util.c list.c getquota.c

getquota_nfs.c: rquota.h
rquota.h: rquota.x
	rpcgen -o $@ -h rquota.x
rquota_xdr.c: rquota.x rquota.h
	rpcgen -o $@ -c rquota.x
rquota_clnt.c: rquota.x rquota.h
	rpcgen -o $@ -l rquota.x
