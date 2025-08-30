#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (fd>=0) close(fd);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        perror("couldn't open file");
        exit(1);
    }
    // 1. Load entire binary content into the memory from the ELF file.
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
      perror("ehdr is faulty");
      exit(1);
    }

    // Magic Number check (Check for ELF File)
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 0x45 ||
        ehdr->e_ident[2] != 0x4c || ehdr->e_ident[3] != 0x46) {
        fprintf(stderr, "Not an ELF file!\n");
        exit(1);
    }

    // Loading the entire program header table
    phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    // Offsetting to start of program header
    if (lseek(fd, ehdr->e_phoff, SEEK_SET) < 0) {
        perror("lseek to program header table failed");
        exit(1);
    }
    if (read(fd, phdr, sizeof(phdr) != sizeof(phdr))) {
        perror("phdr is faulty");
        exit(1);
    }


    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c


    
    // 3. Allocate memory of the size "p_memsz" using mmap function 
    //    and then copy the segment content
    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n",result);
}
