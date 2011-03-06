all: bin/example

bin:
	mkdir bin

bin/example: bin/Makefile
	cd bin ; make

bin/Makefile: bin
	cd bin ; cmake ..

clean:
	rm -rf bin
