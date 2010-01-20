CONFFILE = quota.conf

install-data-local:
	test -f $(DESTDIR)$(sysconfdir)/$(CONFFILE) || \
	      $(top_srcdir)/config/install-sh -m 644 \
	      $(srcdir)/$(CONFFILE) $(DESTDIR)$(sysconfdir)/$(CONFFILE)

EXTRA_DIST = $(CONFFILE)
