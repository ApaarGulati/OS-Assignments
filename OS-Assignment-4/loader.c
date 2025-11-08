#include <signal.h>
#include "loader.h"
#include <stdint.h>
#include <errno.h>

Elf32_Ehdr *ehdr = NULL;            // Pointer for ELF header
Elf32_Phdr *phdr = NULL;            // Pointer for program header
int fd = -1;                        // File descriptor

Elf32_Phdr *ph = NULL;              // Pointer for segment that contains entry point
void *segment_virtual_mem = NULL;   // Pointer to that segment in process memory
unsigned int segment_size = 0;      // Size of the loaded segment in process memory

Elf32_Ehdr *temp_ehdr = NULL;
int temp_fd = -1; 

size_t page_size = 4096;       // fixed page size (4KB)
int total_page_faults = 0;     // total number of segmentation faults triggered
int total_page_allocations = 0; // total pages successfully mapped
size_t internal_fragmentation = 0; // total internal fragmentation in bytes

// Simple linked list to track mapped pages 
typedef struct mapped_page {
    void *base;
    struct mapped_page *next;
} mapped_page_t;
mapped_page_t *mapped_pages_head = NULL;

// Returns 1 if page is already mapped
int is_page_mapped(void *page_base) {
    mapped_page_t *p = mapped_pages_head;
    while (p) { 
        if (p->base == page_base) return 1; 
        p = p->next; 
    }
    return 0;
}

// Add mapped page in the linked list
void mark_page_mapped(void *page_base) {
    mapped_page_t *n = malloc(sizeof(mapped_page_t));
    if (!n) return;
    n->base = page_base;
    n->next = mapped_pages_head;
    mapped_pages_head = n;
}


// Cleanup Functions
void launcher_cleanup() {
  if (temp_fd>=0) close(temp_fd);
  if (temp_ehdr) free(temp_ehdr);
}
void loader_cleanup() {
    // Freeing allocated memory and file descriptor
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (fd>=0) close(fd);

    // Unmapping tracked pages
    mapped_page_t *p = mapped_pages_head;
    while (p) {
        munmap(p->base, page_size);
        mapped_page_t *t = p;
        p = p->next;
        free(t);
    }
    mapped_pages_head = NULL;

    ph = NULL;
}

// Returns the first 20 bits (and makes the remaining ending 12 bits 0)
void* page_align(void* addr) {
    return (void*)((uintptr_t)addr & ~(page_size - 1));
}


size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

// Segments are already page alligned in an ELF so two different segments cannot be in one page
// [ file data ][ BSS (zeros) ][ unused memory ]
// file data -> bytes that actually exist in file (tocopy)
// BSS -> Bytes that belong to the segment in memory but are not in the file (need to be zeroed)
// Unused Memory -> Bytes that lie beyond the segments end (need to counted as fragmentation)

void segfault_handler(int sig, siginfo_t *si, void *unused) {

    // Count a page fault
    total_page_faults++;             

    // Find the virtual address that caused the page fault
    void *fault_addr = si->si_addr;  
    // Find Virtual Address of Page start
    void* page_start = page_align(fault_addr);

    // Already mapped page (meaning this is an actual segmentation fault, and NOT a page fault)
    // Escalate to default handling
    if (is_page_mapped(page_start)) {
        struct sigaction sa_def;
        memset(&sa_def,0,sizeof(sa_def));
        sa_def.sa_handler = SIG_DFL;
        sigaction(SIGSEGV,&sa_def,NULL);
        raise(SIGSEGV);
        return;
    }

    // Determine which PT_LOAD segment contains the faulting page
    Elf32_Phdr *seg = NULL;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uintptr_t seg_start = phdr[i].p_vaddr;
            uintptr_t seg_end   = seg_start + phdr[i].p_memsz;

            if ((uintptr_t)page_start >= seg_start && (uintptr_t)page_start < seg_end) {
                seg = &phdr[i];
                break;
            }
        }
    }

    if (!seg) {
        printf("Segfault at address outside any PT_LOAD segment!\n");
        exit(1);
    }
    
    // Map required page as RW first (permissions are fixed later)
    void* mapped = mmap(page_start, page_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, -1, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    mark_page_mapped(page_start);  // Mark page as mapped
    total_page_allocations++;      // Count allocation


    // Compute offset of this page within the segment
    uintptr_t page_offset_in_segment = (uintptr_t)page_start - seg->p_vaddr;

    // Number of bytes to copy from file
    size_t to_copy = 0;
    if (page_offset_in_segment < seg->p_filesz) {
        to_copy = min(seg->p_filesz - page_offset_in_segment, page_size);
    }

    // Number of bytes to zero (BSS / uninitialized)
    size_t to_zero = 0;
    if (page_offset_in_segment + to_copy < seg->p_memsz) {
        to_zero = min(seg->p_memsz - (page_offset_in_segment + to_copy), page_size - to_copy);
    }

    // Internal fragmentation
    size_t fragmentation = page_size - (to_copy + to_zero);
    internal_fragmentation += fragmentation;

    // Copy file-backed bytes
    if (to_copy > 0) {
        ssize_t n = pread(fd, page_start, to_copy, seg->p_offset + page_offset_in_segment);
        if (n != (ssize_t)to_copy) {
            perror("Failed to read page from file");
            exit(1);
        }
    }

    // Zero out BSS/uninitialized bytes
    if (to_zero > 0) {
        memset((char*)page_start + to_copy, 0, to_zero);
    }


    // Set final permissions
    int prot = 0;
    if (seg->p_flags & PF_R) prot |= PROT_READ;
    if (seg->p_flags & PF_W) prot |= PROT_WRITE;
    if (seg->p_flags & PF_X) prot |= PROT_EXEC;
    mprotect(page_start, page_size, prot);
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

    // Load entire binary content into the memory from the ELF file.
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr){ 
        printf("Error");
        loader_cleanup(); 
        exit(1); 
    }
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("ehdr read failed"); 
        loader_cleanup();
        exit(1);
    }

    // Virual address of the entry point
    unsigned int entry_virtual_add = ehdr->e_entry;

    // Loading the entire program header table
    phdr = malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if (!phdr){ 
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
    // Reading the entire Program header table into memory
    if (read(fd, phdr, sizeof(Elf32_Phdr)*ehdr->e_phnum) != sizeof(Elf32_Phdr)*ehdr->e_phnum) {
        perror("phdr read failed"); 
        loader_cleanup(); 
        exit(1);
    }

    int (*_start)() = (int(*)()) (entry_virtual_add);
    int result = _start();
    printf("User _start return value = %d\n",result);
    printf("Total page faults: %d\n", total_page_faults);
    printf("Total page allocations: %d\n", total_page_allocations);
    printf("Total internal fragmentation: %.2f KB\n", internal_fragmentation / 1024.0);
}




int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: %s <ELF Executable> \n",argv[0]);
        exit(1);
    }

    // Open File
    temp_fd = open(argv[1], O_RDONLY);
    if (temp_fd < 0){ 
        perror("couldn't open file"); 
        launcher_cleanup(); 
        exit(1); 
    }

    // Load the ELF header
    temp_ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!temp_ehdr){ 
        launcher_cleanup(); 
        exit(1); 
    }
    if (read(temp_fd, temp_ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) { 
        perror("ehdr read failed"); 
        launcher_cleanup(); exit(1); 
    }

    // Magic Number check (Check for ELF File)
    if (temp_ehdr->e_ident[0] != 0x7f || temp_ehdr->e_ident[1] != 'E' || 
        temp_ehdr->e_ident[2] != 'L' || temp_ehdr->e_ident[3] != 'F') {
        printf("Not an ELF file!\n"); 
        launcher_cleanup(); 
        exit(1);
    }

    // Cleaning up temp variables
    launcher_cleanup();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;  // Allows us to get faulting address
    sa.sa_sigaction = segfault_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) { 
        perror("sigaction"); 
        exit(1); 
    }

    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}


