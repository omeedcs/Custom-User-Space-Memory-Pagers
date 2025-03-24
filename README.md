# MemPharos: Custom User-Space Memory Pagers

MemPharos is a collection of custom user-space memory pagers for ELF binaries on Linux systems. This project implements different memory paging strategies for loading and executing ELF binaries, providing insights into memory management and paging mechanisms.

## Overview

This project demonstrates several implementations of memory pagers:

- **APager**: Basic ELF loader that maps segments into memory
- **DPager**: Demand paging implementation with SIGSEGV handling for lazy loading
- **HPager**: Hybrid paging approach

Each pager implements different strategies for memory allocation, stack setup, and execution of ELF binaries, showcasing various aspects of user-space memory management.

## Test Programs

The repository includes several test programs to demonstrate the pagers in action:

- `hello_world.c`: Simple "Hello World" program
- `adding_nums.c`: Basic arithmetic operations
- `null.c`: NULL pointer handling
- `data.c`: Data manipulation example
- `crazy_manipulation.c`: More complex data manipulations
- `longstring_longmath.c`: String and mathematical operations
- `extreme_page_faulting.c`: Test case for page fault handling

## Building and Running

Use the provided Makefile to build all pagers and test programs:

```bash
make
```

To run a test program with a specific pager:

```bash
./apager hello_world
./dpager adding_nums
./hpager data
```

## Cleaning up

To clean up compiled binaries:

```bash
make clean
```

## Technical Details

These pagers demonstrate various memory management techniques:

1. **ELF Binary Loading**: Parsing and loading ELF headers and program segments
2. **Stack Management**: Setting up the stack with proper alignment and layout for program execution
3. **Signal Handling**: Intercepting page faults for demand paging
4. **Memory Mapping**: Using mmap/munmap for segment allocation and deallocation

## Requirements

- Linux operating system
- GCC compiler

