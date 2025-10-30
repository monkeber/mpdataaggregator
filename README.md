# Multi-Process Data Aggregator

![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=c%2B%2B&logoColor=white) ![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black) ![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?style=flat&logo=cmake&logoColor=white)

The goal is to design and implement a multi-process application that simulates reading
data from multiple independent sources, aggregates this data into a single, shared,
finite-size buffer, and notifies a separate Consumer process when a complete "batch" of
data is ready for processing.

The solution must be implemented in C or C++ (C++ preferred for modern IPC features),
and the final implementation should be robust, thread-safe (within the processes), and
correctly handle synchronization between the processes.

## Chosen IPC Mechanisms

I've used POSIX robust mutexes for synchronization, POSIX shared memory (`shm_open`) for the buffer and POSIX message queue (`mq_open`) for notifying the consumer about the available data.

## Additional Notes

There are two branches, master contains the ring buffer logic implementation, and the branch `lock_whole_buffer` contains a regular linear buffer, this was the initial solution.

## Clone

For simplicity I used submodules here, so to clone the repo, you need to call:

```bash
git clone --recurse-submodules <project-url>
```

Package managers such as Conan can be used instead of submodules for convenience, but it requires more setup.

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

## Known Issues

There are a couple of bugs that I did not figured out yet:
1. If all the processes are killed unexpectedly, then the allocated shared memory and message queue may stay open in the system, one possible solution is to hold additional robust mutexes in the process that initialized them, then if it dies, another process can detect this and act accordingly.
1. Once when the buffer was created with too many producers (e.g. size of the buffer is 10 and 10 is the number of producers), I managed to freeze my PC, after killing the processes no other process was able to lock the mutex (even though it's robust and that's supposed to be detected) and was blocked indefinitely. Solution for this is manually delete allocated memory:
    ```
    $ ls -l /dev/shm
    $ rm /dev/shm/mpdataaggregator
    ```
1. Not all errors are handled (e.g. while sending the messages), this is a point for improvement.
