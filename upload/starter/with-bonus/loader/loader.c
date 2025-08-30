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
    fd = open(exe[1], O_RDONLY);
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

    // Virual address of the entry point
    unsigned int entry_virtual_add = ehdr->e_entry;

    // Loading the entire program header table
    phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    // Offsetting to start of program header
    if (lseek(fd, ehdr->e_phoff, SEEK_SET) < 0) {
        perror("lseek to program header table failed");
        exit(1);
    }
    if (read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum) != sizeof(Elf32_Phdr) * ehdr->e_phnum) {
        perror("phdr is faulty");
        exit(1);
    }


    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c
    Elf32_Phdr *ph;
    for (int i=0; i<ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            unsigned int start_add = phdr[i].p_vaddr;
            unsigned int end_add = start_add + phdr[i].p_memsz;

            if (start_add<=entry_virtual_add && entry_virtual_add<end_add) {
                ph = &phdr[i];
                break;
            }
        }
    }

    
    // 3. Allocate memory of the size "p_memsz" using mmap function 
    //    and then copy the segment content
    void *segment_virtual_mem;
    segment_virtual_mem = mmap(NULL, ph->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
    if (segment_virtual_mem == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    
    if (lseek(fd, ph->p_offset, SEEK_SET) < 0) {
        perror("lseek to program header failed");
        exit(1);
    }
    if (read(fd, segment_virtual_mem, ph->p_memsz) != ph->p_memsz) {
        perror("PT_LOAD segment is faulty");
        exit(1);
    }

    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
    unsigned int entry_offset = entry_virtual_add - ph->p_vaddr;
    unsigned int entry_address = (unsigned int)segment_virtual_mem + entry_offset;
    
    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    int (*_start)() = (int(*)()) (entry_address);

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n",result);
}

int main(int argc, char **argv){

    if (argc != 2){
        printf("usage: %s binary_file\n", argv[0]);
        return 1;
    }

    load_and_run_elf(argv);
    loader_cleanup();

    return 0;
}