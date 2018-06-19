SUBDIRS = liblsd libutil librpc

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
	$(top_builddir)/src/liblsd/liblsd.a \
	$(top_builddir)/src/libutil/libutil.a \
	$(top_builddir)/src/librpc/librpc.a

common_sources = \
	getquota.c \
	getquota.h \
	getquota_private.h \
	getquota_nfs.c \
	getquota_lustre.c
