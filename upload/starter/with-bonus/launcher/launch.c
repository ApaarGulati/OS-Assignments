#include "loader.h"

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  Elf32_Ehdr *ehdr;
  int fd;     
  // Open file
  fd = open(argv[1], O_RDONLY);
  if (fd < 0){
    perror("couldn't open file");
    loader_cleanup();
    exit(1);
  }
  
  // Load the ELF header
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
  
  // Magic Number check (Check for ELF File)
  if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 0x45 ||
    ehdr->e_ident[2] != 0x4c || ehdr->e_ident[3] != 0x46) {
    printf("Not an ELF file!\n");
    loader_cleanup();
    exit(1);
  }
    
  close(fd);
  free(ehdr);

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
