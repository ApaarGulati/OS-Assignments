#include "loader.h"

Elf32_Ehdr *ehdr;            // Pointer for ELF header
Elf32_Phdr *phdr;            // Pointer for program header
int fd;                      // File descriptor

Elf32_Phdr *ph;              // Pointer for segment that contains entry point
void *segment_virtual_mem;   // Pointer to that segment in process memory
unsigned int segment_size;   // Size of the loaded segment in process memory

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    // Freeing allocated memory and file descriptor
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (fd>=0) close(fd);

    ph = NULL;
    
    // Unmapping mapped memory and freeing
    if (segment_virtual_mem) {
        munmap(segment_virtual_mem, segment_size);
        segment_virtual_mem = NULL;
    }

}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {

    // Opening file descriptor
    fd = open(exe[1], O_RDONLY);
    if (fd < 0){
        perror("couldn't open file");
        loader_cleanup();
        exit(1);
    }
    // 1. Load entire binary content into the memory from the ELF file.
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL){
        printf("Error");
        loader_cleanup();
        exit(1);
    }
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
      perror("ehdr is faulty");
      loader_cleanup();
      exit(1);
    }


    // Virual address of the entry point
    unsigned int entry_virtual_add = ehdr->e_entry;

    // Loading the entire program header table
    phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if (phdr == NULL){
        printf("Error");
        loader_cleanup();
        exit(1);
    }
    // Offsetting to start of program header
    if (lseek(fd, ehdr->e_phoff, SEEK_SET) < 0) {
        perror("lseek to program header table failed");
        loader_cleanup();
        exit(1);
    }
    if (read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum) != sizeof(Elf32_Phdr) * ehdr->e_phnum) {
        perror("phdr is faulty");
        loader_cleanup();
        exit(1);
    }


    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c
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
    segment_size = ph->p_memsz;
    segment_virtual_mem = mmap(NULL, segment_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
    if (segment_virtual_mem == MAP_FAILED) {
        perror("mmap failed");
        loader_cleanup();
        exit(1);
    }
    
    if (lseek(fd, ph->p_offset, SEEK_SET) < 0) {
        perror("lseek to program header failed");
        loader_cleanup();
        exit(1);
    }
    if (read(fd, segment_virtual_mem, segment_size) != segment_size) {
        perror("PT_LOAD segment is faulty");
        loader_cleanup();
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