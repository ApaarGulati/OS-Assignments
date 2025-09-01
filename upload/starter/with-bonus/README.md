# OS Assignment - 1
## Linker and Loader

### Loader
The **loader** takes in an executable file as input. It then :

1. Processes the ELF header, storing the entry point virtual address. 
2. It then goes to the program header table (using the offset from ELF header).
3. It iterates on each entry of this table and checks which **PT_LOAD** segment contains the entry point virtual address and loads that segment into the mapped memory. 
4. It then calculates where the entry point lies within this segment (i.e., find the offset of the entry point from the start of the segment). 
5. It then uses this offset to find the actual entry point inside the loaded segment in the mapped memory. 
6. Then it just makes a function pointer (`_start`) to point at the entry point and call `_start()` (mapped to libc main) at the entry point.

---

### Launcher
The **launcher**: 
1. Checks the validity of the executable file (using the ELF magic number).
2. It then sends it over to the loader for loading and executing the program.
3. Then it calls the loader_cleanup() function to clean up all the allocated memory/pointers/variables in the loader.

--- 

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignment-1