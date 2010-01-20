check_PROGRAMS = tconf
TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_QUOTA=$(top_builddir)/src/quota"
TESTS_ENVIRONMENT += "PATH_REPQUOTA=$(top_builddir)/src/repquota"
TESTS = runtests

CLEANFILES = *.out *.diff x.conf

AM_CFLAGS = -I$(top_srcdir)/src

tconf_SOURCES = tconf.c \
		$(top_srcdir)/src/getconf.c \
		$(top_srcdir)/src/list.c \
		$(top_srcdir)/src/util.c
