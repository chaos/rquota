SUFFIXES = .x

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
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
