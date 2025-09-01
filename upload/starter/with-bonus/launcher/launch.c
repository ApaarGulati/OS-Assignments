#include "loader.h"

Elf32_Ehdr *temp_ehdr = NULL;
int temp_fd = -1; 


void launcher_cleanup() {
  if (temp_fd>=0) close(temp_fd);
  if (temp_ehdr) free(temp_ehdr);
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  // 1. carry out necessary checks on the input ELF file    
  // Open file
  temp_fd = open(argv[1], O_RDONLY);
  if (temp_fd < 0){
    perror("couldn't open file");
    launcher_cleanup();
    exit(1);
  }
  
  // Load the ELF header
  temp_ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
  if (temp_ehdr == NULL){
      printf("Error");
      launcher_cleanup();
      exit(1);
  }
  if (read(temp_fd, temp_ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
    perror("ehdr is faulty");
    launcher_cleanup();
    exit(1);
  }
  
  // Magic Number check (Check for ELF File)
  if (temp_ehdr->e_ident[0] != 0x7f || temp_ehdr->e_ident[1] != 0x45 ||
    temp_ehdr->e_ident[2] != 0x4c || temp_ehdr->e_ident[3] != 0x46) {
    printf("Not an ELF file!\n");
    launcher_cleanup();
    exit(1);
  }
  // Cleaning up temp variables
  launcher_cleanup();
  

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
