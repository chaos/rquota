SUBDIRS = liblsd

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-D_PATH_QUOTA_CONF=\"@X_SYSCONFDIR@/quota.conf\" \
	-I$(top_srcdir)

bin_PROGRAMS = quota repquota

quota_SOURCES = quota.c $(common_sources)
quota_LDADD = $(common_ldadd)

repquota_SOURCES = repquota.c $(common_sources)
repquota_LDADD = $(common_ldadd)


common_ldadd = \
	$(top_builddir)/src/liblsd/liblsd.a

common_sources = \
  getquota.c getquota.h getquota_private.h getquota_nfs.c getquota_lustre.c \
  util.c util.h getconf.c getconf.h \
  rquota_xdr.c rquota_clnt.c rquota.h listint.c listint.h

CLEANFILES = rquota.h rquota_xdr.c rquota_clnt.c

getquota_nfs.c: rquota.h
rquota.h: rquota.x
	rpcgen -o $@ -h rquota.x
rquota_xdr.c: rquota.x rquota.h
	rpcgen -o $@ -c rquota.x
rquota_clnt.c: rquota.x rquota.h
	rpcgen -o $@ -l rquota.x

EXTRA_DIST = rquota.x
