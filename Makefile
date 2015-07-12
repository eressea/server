all:
	s/build

test:
	s/runtests

clean:
	@rm -f *.log.*
	@find . -name "*~" | xargs rm -f
