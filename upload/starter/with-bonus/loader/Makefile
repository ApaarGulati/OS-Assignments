#Create lib_simpleloader.so from loader.c
all: ../bin/lib_simpleloader.so

../bin/lib_simpleloader.so: loader.c loader.h
	gcc -m32 -fPIC -shared -o lib_simpleloader.so loader.c
# -fPIC -> forced Position Independent Code (so code can be loaded in any arbitrary memory address and still execute correctly)
# -shared -> Used to create shared objects/libraries
#Provide the command for cleanup
clean:
	-@rm -f ../bin/lib_simpleloader.so