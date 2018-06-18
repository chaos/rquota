check_PROGRAMS = tconf

dist_check_SCRIPTS = runtests \
	00.sh 01.sh 02.sh 03.sh 04.sh 05.sh 06.sh \
	07.sh 08.sh 09.sh 10.sh 11.sh 12.sh 13.sh 14.sh

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_QUOTA=$(top_builddir)/src/quota"
TESTS_ENVIRONMENT += "PATH_REPQUOTA=$(top_builddir)/src/repquota"

TESTS = runtests

CLEANFILES = *.out *.diff x.conf

AM_CFLAGS = -I$(top_srcdir)/src

tconf_SOURCES = tconf.c
tconf_LDADD = \
	$(top_builddir)/src/getconf.o \
	$(top_builddir)/src/list.o \
	$(top_builddir)/src/util.o

EXTRA_DIST = \
	00.exp 01.exp 02.exp 03.exp 04.exp 05.exp 06.exp \
	07.exp 08.exp 09.exp 10.exp 11.exp 12.exp 13.exp 14.exp
