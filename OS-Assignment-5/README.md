# OS Assignment - 5
## Simple Multithreader 


This is a **simple multithreader**, which is just implemented as a C++ header file using **Pthreads** which helps us parallelize code for **1D Vector traversal** and **2D matrix traversal** using **lambda functions**. We break the loops into smaller parts and use separate threads for each part which helps us parallelize the computation and make it faster.

For **1D vectors**, we break the vector into smaller size vectors such that each thread (including the **parent process**) gets approximately the same sized chunk of the vector to work on (accounting for **remaining size** and distributing it equally among all the threads).

For **2D matrices**, we break the matrix into smaller matrix (only according to **outer loop / rows**) such that each thread (including the **parent process**) gets approximately the same sized chunk / same number of rows to work on (accounting for the **remaining size / rows** and distributing it equally among all the threads).

We make sure that the **parent thread also contributes to actually doing work**, so we create an extra **`(n-1) threads`** in the parent process for a total of **`n` parallelly executing programs** (given number of cores is larger, otherwise it just caps at number of cores).




---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/OS-Assignment-5