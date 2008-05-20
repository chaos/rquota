TESTS	= 01 02 03 04 05 06
QUOTA=../quota
REPQUOTA=../repquota

check:
	$(QUOTA) --selftest
	./runtests $(QUOTA) $(REPQUOTA) $(TESTS)

clean:
	rm -f *.diff *.out
	rm -f x.conf
