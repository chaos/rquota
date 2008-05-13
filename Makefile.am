AM_CFLAGS = -Wall -Wno-unused

bin_PROGRAMS = quota repquota

quota_SOURCES = quota.c $(common_sources)
repquota_SOURCES = repquota.c $(common_sources)

common_sources = \
	getquota.c getquota_nfs.c getquota_lustre.c \
	util.c list.c getconf.c rquota_xdr.c rquota_clnt.c

getquota_nfs.c: rquota.h
rquota.h: rquota.x
	rpcgen -o $@ -h rquota.x
rquota_xdr.c: rquota.x rquota.h
	rpcgen -o $@ -c rquota.x
rquota_clnt.c: rquota.x rquota.h
	rpcgen -o $@ -l rquota.x
