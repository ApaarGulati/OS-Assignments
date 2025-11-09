# OS Assignment - 3
## Simple Smart Loader 

This is a **simple smart loader**, which is an upgrade to the **simple loader (Assignment 1)** as it supports **Dynamic Page Loading**, i.e., pages are **lazily loaded** as needed. Whenever a memory access is required, we first check if the required page is available in memory. If it is not available, it raises a **segmentation fault**, which will be handled as a **page fault**. We handle the page fault by performing necessary **permission checks** and then successfully **loading the required page into memory**. After this, control is returned back to the program.
We can see that a segment in ELF is from `p_offset` till `p_offset + p_filesz` but in memory, it is from `p_vaddr` till `p_vaddr + p_memsz` (where **p_offset % p_align == p_vaddr % p_align** and **p_filesz <= p_memsz**). In simple language, the ELF file content is initialized data, but when it is loaded into memory, it may have some uninitialized data (**p_memsize - p_filesz**) that needs to be allocated (but zeroed out initially). 
A page can contain some **initialized data**, which is loaded from the ELF file, some **uninitialized data**, which is set to **zero**, or some **unused space**, which is not a part of any segment.
We have found the **overlapping intersection** of the page and these segments and these correspondingly loaded the appropriate data from the file OR zeroed it out in case of uninitialized data OR counted it as fragmentation (unused memory in allocated page).




---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/OS-Assignment-4