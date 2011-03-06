PROJECT = eressea

all: bin/$(PROJECT)

bin:
	mkdir bin

bin/$(PROJECT): bin/Makefile
	cd bin ; make

bin/Makefile: bin
	cd bin ; cmake ..

clean:
	rm -rf bin
