SUFFIXES = .x

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I/usr/include/tirpc \
	-include rquota.h

noinst_LIBRARIES = librpc.a

librpc_a_SOURCES = \
	rquota.x

nodist_librpc_a_SOURCES = \
	rquota.h \
	rquota_xdr.c \
	rquota_clnt.c

CLEANFILES = rquota.h rquota_xdr.c rquota_clnt.c

rquota_xdr.o: rquota.h
rquota_clnt.o: rquota.h

rquota.h: rquota.x
	$(RPCGEN) -o $@ -h <$<
rquota_xdr.c: rquota.x
	$(RPCGEN) -o $@ -c <$<
rquota_clnt.c: rquota.x
	$(RPCGEN) -o $@ -l <$<

#
# Test client/server
#
check_PROGRAMS = \
	rquota_svc_test \
	rquota_clnt_test

rquota_clnt_test_SOURCES = \
	rquota_clnt_test.c
rquota_clnt_test_LDADD = librpc.a

rquota_svc_test_SOURCES = \
	rquota_svc_main.c \
	rquota_svc_test.c
rquota_svc_test_LDADD = librpc.a

rquota_svc_main.o: rquota.h

rquota_svc_main.c: rquota.x
	$(RPCGEN) -o $@ -s udp -DTEST <$<

CLEANFILES += rquota_svc_main.c
