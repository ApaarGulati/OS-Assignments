#invoke make inside following directories and in this order: loader, launch, fib
#move the lib_simpleloader.so and launch binaries inside bin directory
all: bin/lib_simpleloader.so bin/launch test/fib
	./bin/launch ./test/fib

bin/lib_simpleloader.so:
	make -C loader
	mv loader/lib_simpleloader.so bin/

bin/launch:
	make -C launcher
	mv launcher/launch bin/

test/fib:
	make -C test

#Provide the command for cleanup
clean:
	make -C loader clean
	make -C launcher clean
	make -C test clean
