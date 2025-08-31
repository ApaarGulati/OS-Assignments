#invoke make inside following directories and in this order: loader, launch, fib
all:
	make -C loader
	make -C test
	./loader/loader ./test/fib

#move the lib_simpleloader.so and launch binaries inside bin directory
#Provide the command for cleanup
clean:
	make -C loader clean
	make -C test clean
