all:
	s/build

test:
	s/runtests

clean:
	@rm -rf *.log.* Debug/CMake* 
	@find . -name "*~" | xargs rm -f
