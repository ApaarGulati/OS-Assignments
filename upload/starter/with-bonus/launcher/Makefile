#Compile the launch.c by linking it with the lib_simpleloader.so
all: ../bin/launch

../bin/launch: launch.c ../loader/loader.h ../bin/lib_simpleloader.so
	gcc -m32 -o launch launch.c -I../loader -L../bin -l_simpleloader -Wl,-rpath,'$$ORIGIN'

# By default, during compile time gcc searches the current directory and default system directories for headers
# -I../loader -> Add ../loader/ to the list of places gcc looks for headers

# (FOR COMPILE TIME LINKER)
# By default, gcc searches the standard system library paths at compile time
# -L../bin -> Add ../bin to the library search path
# -l_simpleloader -> Link against the library named lib_simpleloader.so, gcc automatically prepends lib and appends .so

# (FOR RUN TIME LINKER)
# -Wl,-rpath,'$$ORIGIN' -> tells the binary to look for .so in the same folder as the binary at runtime
#    -> -Wl passes the following option to the linker
#    -> -rpath sets the runtime library search path for this binary
#    -> At runtime, $$ORIGIN is replaced with the same directory containing the executable.

#Provide the command for cleanup
clean:
	-@rm -f ../bin/launch
