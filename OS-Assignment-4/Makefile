all: loader

fib: fib.c
	gcc -m32 -no-pie -nostdlib -o fib fib.c

sum: sum.c
	gcc -m32 -no-pie -nostdlib -o sum sum.c

loader: loader.c
	gcc -m32 -o loader loader.c

run-fib: fib loader
	./loader fib

run-sum: sum loader
	./loader sum

clean:
	rm -f fib sum loader
