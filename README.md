# Operating Systems Assignments

This repository contains the assignments completed for the Operating Systems course. The project is divided into five distinct assignments, each focusing on core OS concepts, ranging from linkers and loaders to simple shells, schedulers, and multithreading.

## Table of Contents
- [Assignment 1: Linker and Loader](#assignment-1-linker-and-loader)
- [Assignment 2: Simple Shell](#assignment-2-simple-shell)
- [Assignment 3: Simple Scheduler](#assignment-3-simple-scheduler)
- [Assignment 4: Simple Smart Loader](#assignment-4-simple-smart-loader)
- [Assignment 5: Simple Multithreader](#assignment-5-simple-multithreader)

---

## Assignment 1: Linker and Loader
**Directory:** `OS-Assignment-1`

### Loader
The **loader** takes an executable file as input and performs the following:
1. Processes the ELF header, storing the entry point virtual address. 
2. Navigates to the program header table (using the offset from the ELF header).
3. Iterates through each entry to check which **PT_LOAD** segment contains the entry point virtual address, and loads that segment into mapped memory. 
4. Calculates where the entry point lies within this segment (offset from the segment start). 
5. Uses this offset to locate the actual entry point inside the loaded segment in the mapped memory. 
6. Creates a function pointer (`_start`) pointing to the entry point and calls `_start()` (mapped to `libc` main).

### Launcher
The **launcher**: 
1. Verifies the validity of the executable file using the ELF magic number.
2. Forwards it to the loader for execution.
3. Calls the `loader_cleanup()` function to clean up all allocated memory, pointers, and variables in the loader.

---

## Assignment 2: Simple Shell
**Directory:** `OS-Assignment-2`

This is a simple shell that executes basic commands (defined in `/bin`). Note that it does not implement built-in commands like `cd` and instead provides a user-friendly error message.  

**Key features:**
- **Pipes & File Descriptors:** Supports piping using the `pipe()` system call. We split the input about the pipes (`|`), create the pipes, and connect them by replacing `stdin`/`stdout` of the child processes.
- **Process Management:** Each command is executed within a child process using `fork()` and `execvp()`. 
- **Command History:** We store each command in the history with information like its PID, execution time, etc. It handles commands like `history` and prints the history upon termination (via `Ctrl+C`).

---

## Assignment 3: Simple Scheduler
**Directory:** `OS-Assignment-3`

This is a simple scheduler that executes simple executables via a **submit** command. Executables must not have blocking calls, command-line arguments, or pipes. The scheduler is integrated with the **Simple Shell** from Assignment 2.  

**Key features:**
- **Round-Robin Scheduling:** The 'virtual computer' has **N CPUs** and a **T time slice**. It runs up to **N processes** simultaneously for the given time slice, then puts them back into the ready queue in a round-robin fashion.
- **Shared Memory & Synchronization:** Process information is stored in shared memory accessed by both the shell and the scheduler. Proper **semaphores and locks** are implemented to avoid race conditions and deadlocks. 
- **Graceful Shutdown:** A **shutdown flag** in the shared memory is set by the shell upon termination. The scheduler checks this flag in each iteration; if set, it terminates all running jobs and exits.
- **Compatibility:** A **`dummy_main.h`** header file is included in all executables to replace their actual `main` function, allowing the executable to be sent to the ready queue rather than executing immediately.

---

## Assignment 4: Simple Smart Loader 
**Directory:** `OS-Assignment-4`

An upgrade to the Simple Loader (Assignment 1) that supports **Dynamic Page Loading**. Pages are lazily loaded as needed rather than all at once.

**Key features:**
- **Page Fault Handling:** If a memory access requires a page not yet loaded, a segmentation fault is raised and caught. We handle this page fault by performing permission checks and loading the required page into memory, before returning control to the program.
- **Memory Mapping:** A segment in the ELF file (from `p_offset` to `p_offset + p_filesz`) is mapped into memory (from `p_vaddr` to `p_vaddr + p_memsz`). Any uninitialized data is allocated and zeroed out initially. 
- **Overlap Resolution:** We identify the overlapping intersection of requested pages and ELF segments. We load the appropriate data from the file, zero it out (for uninitialized data), or count it as fragmentation (unused memory in an allocated page).

---

## Assignment 5: Simple Multithreader 
**Directory:** `OS-Assignment-5`

This is a **simple multithreader** implemented as a C++ header file using **Pthreads**, allowing parallelization of 1D vector and 2D matrix traversals via **lambda functions**.

**Key features:**
- **1D Vector Traversal:** The vector is split into equal-sized chunks among threads (including the parent process), distributing any remaining size evenly.
- **2D Matrix Traversal:** The matrix is split by rows (outer loop) such that each thread receives an approximately equal chunk of rows to process.
- **Worker Optimization:** The parent thread also contributes to the workload. We create `(n-1)` additional threads for a total of `n` parallel workers (capped at the number of available cores).

---

## Group 44
The work was equally divided with all aspects of the assignments completed together.

- **Apaar Gulati** - 2024095
- **Bhavya Yadav** - 2024152

## Github Repository
[OS-Assignments Github Repo](https://github.com/ApaarGulati/OS-Assignments)
