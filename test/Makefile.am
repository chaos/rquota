check_PROGRAMS = tconf
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

EXTRA_DIST = $(TESTS) *.sh *.exp
