# OS Assignment - 3
## Simple Smart Loader 

# Simple Smart Loader

This is a **simple smart loader**, which is an upgrade to the **simple loader (Assignment 1)** as it supports **demand paging**, i.e., pages are **lazily loaded** as needed. Whenever a memory access is required, we first check if the required page is available in memory. If it is not available, it raises a **segmentation fault**, which will be handled as a **page fault**. We handle the page fault by performing necessary **permission checks** and then successfully **loading the required page into memory**. After this, control is returned back to the program.

A page can contain some **initialized data**, which is loaded from the ELF file, some **uninitialized data**, which is set to **zero**, or some **unused space**, which is not a part of any segment.

Segments in ELF files are **page-aligned** (`p_align`), which ensures that a page cannot contain **two segments**, avoiding potential **permission issues**. A segment can only start from a **multiple of the page size**.




---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/OS-Assignment-4