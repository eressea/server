all: tests

tests: bin/server
	@bin/server -l/dev/null --tests

bin:
	@mkdir bin

bin/server: bin/Makefile
	cd bin ; make

bin/Makefile: CMakeLists.txt | bin
	cd bin ; cmake ..

clean:
	rm -rf bin
