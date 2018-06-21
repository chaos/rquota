AM_CFLAGS = @GCCWARN@
AM_CPPFLAGS = -I$(top_srcdir)

check_PROGRAMS = tconf

dist_check_SCRIPTS = 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_QUOTA=$(top_builddir)/src/cmd/quota"
TESTS_ENVIRONMENT += "PATH_REPQUOTA=$(top_builddir)/src/cmd/repquota"
TESTS_ENVIRONMENT += "TEST_BUILDDIR=$(builddir)"
TESTS_ENVIRONMENT += "TEST_SRCDIR=$(srcdir)"

TESTS = $(dist_check_SCRIPTS)

CLEANFILES = *.out *.diff *.conf

tconf_SOURCES = tconf.c
tconf_LDADD = \
	$(top_builddir)/src/libutil/libutil.a \
	$(top_builddir)/src/liblsd/liblsd.a

EXTRA_DIST = \
	00.exp 01.exp 02.exp 03.exp 04.exp 05.exp 06.exp \
	07.exp 08.exp 09.exp 10.exp 11.exp 12.exp 13.exp 14.exp
