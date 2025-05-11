# Mem-Allocator
This project includes one primary files - allocator.c. The file contains methods to request, release, compact, and view statistics about a sized block of memory allocated at runtime. Additionally, it contains strategies for allocating memory holes in various strategies such as worst fit, best fit, and first fit.

## Source Files

* allocator.c
* README.md

## References

* man pages
* c documentation

## Known Errors

* n/a

## Build Insructions (2 Separate Terminals)

* gcc allocator.c -o alloctor

## Execution Instructions (2 Separate Terminals)

* ./allocator <mem_size>

## Sample Output:

./allocator 1048576
allocator>RQ P1 4000 B 
allocator>STAT
Addresses [0:3999] Process P1
Addresses [4000:1048575] Unused
allocator>RL P1
allocator>
allocator>STAT
Addresses [0:1048575] Unused
allocator>X

