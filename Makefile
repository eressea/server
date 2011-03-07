all: bin/eressea

bin:
	mkdir bin

bin/eressea: bin/Makefile
	cd bin ; make

bin/Makefile: bin
	cd bin ; cmake ..

clean:
	rm -rf bin
