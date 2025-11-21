
// Making sure we only include header file once to omit errors
#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H

#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include <algorithm>
#include <cassert>


// User main wrapper
int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
    lambda();
}


// Thread argument structs (argument passed to p_thread)
struct ThreadArgs1D {
    int low;
    int high;
    std::function<void(int)> *lambda;
};

struct ThreadArgs2D {
    int low1, high1;
    int low2, high2;
    std::function<void(int,int)> *lambda;
};


// Thread functions (functions passed to p_thread)
void* thread_func_1d(void* arg) {
    ThreadArgs1D* args = (ThreadArgs1D*)arg;
    for (int i = args->low; i < args->high; i++) {
        (*(args->lambda))(i);
    }
    return nullptr;
}

void* thread_func_2d(void* arg) {
    ThreadArgs2D* args = (ThreadArgs2D*)arg;
    for (int i = args->low1; i < args->high1; i++) {
        for (int j = args->low2; j < args->high2; j++) {
            (*(args->lambda))(i, j);
        }
    }
    return nullptr;
}


// Validate number of threads (helper)
// Inline function to reduce overhead of actual function call
inline void validate_numThreads(int numThreads) {
    if (numThreads <= 0) {
        std::cerr << "[Error] Number of threads must be >= 1\n";
        exit(1);
    }
}


// 1D parallel_for
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
    // Validate thread number
    validate_numThreads(numThreads);

    // Validate chunk
    if (low >= high) return; 

    // Check start time
    auto start_time = std::chrono::high_resolution_clock::now();

    // Store all the new (n-1) threads
    pthread_t *threads = new pthread_t[numThreads - 1];
    // Store all the arguments for every single thread (along with parent process)
    ThreadArgs1D *args = new ThreadArgs1D[numThreads];

    int total = high - low;
    int base = total / numThreads;    // Chunk size per thread
    int rem  = total % numThreads;    // Remainder chunk size

    // Main thread handles first chunk
    int firstChunk = base;
    if (0<rem) firstChunk++;
    
    args[0].low = low;
    args[0].high = std::min(high, low + firstChunk);
    args[0].lambda = &lambda;
    thread_func_1d(&args[0]);

    // Start index for remaining threads
    int start = args[0].high;
    // Iterating through the n-1 threads we create
    for (int t = 1; t < numThreads; t++) {
        int chunk = base;    // Determining chunk size 
        if (t<rem) chunk++;  // Accounting for remainder chunk

        int myLow = start;
        int myHigh = std::min(high, start + chunk);

        args[t].low = myLow;
        args[t].high = myHigh;
        args[t].lambda = &lambda;

        if (pthread_create(&threads[t-1], nullptr, thread_func_1d, &args[t]) != 0) {
            perror("[Error] pthread_create failed");
            exit(1);
        }

        start = myHigh;
    }

    // Joining all the threads back
    for (int t = 1; t < numThreads; t++) {
        pthread_join(threads[t-1], nullptr);
    }

    // Checking end time
    auto end_time = std::chrono::high_resolution_clock::now();
    // Calculating total time taken
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "[parallel_for 1D] Execution Time = " << ms << " ms\n";

    // Cleanup
    delete[] threads;
    delete[] args;
}


// 2D parallel_for
void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int,int)> &&lambda, int numThreads) {
    // Validate thread number
    validate_numThreads(numThreads);

    // Validate chunk size
    if (low1 >= high1 || low2 >= high2) return;

    // Check start time
    auto start_time = std::chrono::high_resolution_clock::now();

    // Store all the new (n-1) threads
    pthread_t *threads = new pthread_t[numThreads - 1];
    // Store all the arguments for every single thread (along with the parent process)
    ThreadArgs2D *args = new ThreadArgs2D[numThreads];

    int total = high1 - low1;
    int base = total / numThreads;   // Chunk size per thread
    int rem  = total % numThreads;   // Remainder chunk size

    // Main thread handles first chunk
    int firstChunk = base;
    if (0<rem) firstChunk++;

    args[0].low1 = low1;
    args[0].high1 = std::min(high1, low1 + firstChunk);
    args[0].low2 = low2;
    args[0].high2 = high2;
    args[0].lambda = &lambda;
    thread_func_2d(&args[0]);

    // Start index for remaining threads
    int start = args[0].high1;
    // Iterating through the n-1 threads we create
    for (int t = 1; t < numThreads; t++) {
        int chunk = base;    // Determining chunk size 
        if (t<rem) chunk++;  // Accounting for remainder chunk

        int myLow1 = start;
        int myHigh1 = std::min(high1, start + chunk);

        args[t].low1 = myLow1;
        args[t].high1 = myHigh1;
        args[t].low2 = low2;
        args[t].high2 = high2;
        args[t].lambda = &lambda;

        if (pthread_create(&threads[t-1], nullptr, thread_func_2d, &args[t]) != 0) {
            perror("[Error] pthread_create failed");
            exit(1);
        }

        start = myHigh1;
    }


    // Joining all the threads back
    for (int t = 1; t < numThreads; t++) {
        pthread_join(threads[t-1], nullptr);
    }

    // Checking end time
    auto end_time = std::chrono::high_resolution_clock::now();
    // Calculating total time taken
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "[parallel_for 2D] Execution Time = " << ms << " ms\n";

    // Cleanup
    delete[] threads;
    delete[] args;
}


// main wrapper
int main(int argc, char **argv) {
    int x=5,y=1;
    auto lambda1 = [x,&y](void) {
        y = 5;
        std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    };
    demonstration(lambda1);


    // Input validation for user program
    if (argc < 2) {
        std::cerr << "Usage: ./program <numThreads> [problemSize]\n";
        exit(1);
    }

    int numThreads = atoi(argv[1]);
    if (numThreads <= 0) {
        std::cerr << "[Error] numThreads must be > 0\n";
        exit(1);
    }

    int rc = user_main(argc, argv);

    auto lambda2 = []() {
        std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    };
    demonstration(lambda2);
    return rc;
}

#define main user_main

#endif
