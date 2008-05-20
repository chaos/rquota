TESTS	= 01
QUOTA=../quota
REPQUOTA=../repquota

check:
	$(QUOTA) --selftest
	./runtests $(QUOTA) $(REPQUOTA) $(TESTS)

clean:
	rm -f *.diff *.out
	rm -f x.conf
