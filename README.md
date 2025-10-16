# Multi-Process Data Aggregator

The goal is to design and implement a multi-process application that simulates reading
data from multiple independent sources, aggregates this data into a single, shared,
finite-size buffer, and notifies a separate Consumer process when a complete "batch" of
data is ready for processing.

The solution must be implemented in C or C++ (C++ preferred for modern IPC features),
and the final implementation should be robust, thread-safe (within the processes), and
correctly handle synchronization between the processes.

## Configure And Build

First configure the project:
```bash
cmake --preset debug
```
And then build:
```bash
cmake --build build --preset debug
```

## Tools Used

I used such tools for the project, I think it should work with older versions as well if they are not too outdated:
```bash
$ cmake --version
cmake version 4.1.2

$ gcc --version
gcc (GCC) 15.2.1 20250813

$ clang-format --version
clang-format version 20.1.8
```
