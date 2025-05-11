#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h> // for INT_MAX, large initial integer for best fit

#define NAME_LEN 32 // max process name length
#define CMD_LEN 128 // max command line length

typedef struct Block {
    int start;               // starting address
    int size;                // size in bytes
    bool free;               // is block free
    char name[NAME_LEN];     // process name if allocated
    struct Block *next;      // pointer to next Block
} Block;

static Block *head = NULL;   // head of the memory‐segment list
static int total_mem = 0;    // total size

void status() {
    // initializes head, checks if p is not null, executes loop body, iterates to next Block
    for (Block *p = head; p; p = p->next) {
        int end = p->start + p->size - 1;
        if (p->free)
            printf("Addresses [%d:%d] Unused\n", p->start, end);
        else
            printf("Addresses [%d:%d] Process %s\n", p->start, end, p->name);
    }
}

// allocate bytes to a process name, using strategy F B or W
void allocate_mem(const char *name, int size, char strat) {
    // check for taken process name
    for (Block *p = head; p; p = p->next) {
        if (!p->free && strcmp(p->name, name) == 0) {
            printf("Error: Process %s already exists\n", name);
            return;
        }
    }

    // best is current best choice to satisfy request
    // best_prev keeps track of node before best for when you split or replace best
    // prev keeps track of node behind p
    Block *best = NULL, *best_prev = NULL, *prev = NULL;
    
    // best_metric if strategy is Best Fit
    int best_metric;
    if (strat == 'B') {
        best_metric = INT_MAX;
    } else {
        best_metric = 0;
    }

    for (Block *p = head; p; prev = p, p = p->next) {
        // iterates through block, prev trails behind

        // skip if alr allocatedd or too small
        if (!p->free || p->size < size) {
            continue;       
        }
        if (strat == 'F') {
            best = p; // first fit pick this hole
            best_prev = prev; // track previous
            break;
        } 
        else if (strat == 'B' && p->size < best_metric) {
            best_metric = p->size; // found smaller hole
            best = p; // update best (so far)
            best_prev = prev; // track previous
        } 
        else if (strat == 'W' && p->size > best_metric) {
            best_metric = p->size; // found larger hole
            best = p; // update best (so far)
            best_prev = prev; // track previous
        }
    }
    // no suitable hole
    if (!best) {
        printf("Error: Not enough memory for process %s\n", name);
        return;
    }
    // check for exact fit
    if (best->size == size) {
        best->free = false;
        strcpy(best->name, name);
    } else {
        // split hole
        Block *alloc = malloc(sizeof *alloc);
        alloc->start = best->start;
        alloc->size  = size;
        alloc->free  = false;
        strcpy(alloc->name, name);

        // shrink hole
        best->start += size;
        best->size  -= size;

        // insert before best
        if (best_prev)
            best_prev->next = alloc;
        else
            head = alloc;
        alloc->next = best;
    }
}

// release mem held by process P and merge adjacent holes
void release_mem(const char *name) {
    Block *p = head, *prev = NULL;
    while (p && (p->free || strcmp(p->name, name) != 0)) {
        prev = p;
        p = p->next;
    }
    if (!p) {
        printf("Error: Process %s not found\n", name);
        return;
    }
    p->free = true;
    p->name[0] = '\0';

    // merge with previous
    if (prev && prev->free) {
        prev->size += p->size;
        prev->next  = p->next;
        free(p);
        p = prev;
    }

    // merge with next
    if (p->next && p->next->free) {
        Block *n = p->next;
        p->size += n->size;
        p->next  = n->next;
        free(n);
    }
}

// compact the free memory into one larger memory hole
void compact_mem() {
    Block *new_head = NULL, *tail = NULL;
    int cur = 0;
    // copy alloc blocks, in order
    for (Block *p = head; p; p = p->next) {
        if (!p->free) { // check if not free
            Block *nb = malloc(sizeof *nb); // struct to compact
            nb->start = cur;
            nb->size  = p->size;
            nb->free  = false;
            strcpy(nb->name, p->name);
            nb->next  = NULL;
            if (!new_head) new_head = tail = nb;
            else { tail->next = nb; tail = nb; }
            cur += nb->size;
        }
    }
    // single big hole
    if (cur < total_mem) {
        Block *nb = malloc(sizeof *nb); // struct to compact
        nb->start = cur;
        nb->size  = total_mem - cur;
        nb->free  = true;
        nb->name[0] = '\0';
        nb->next = NULL;
        if (!new_head) new_head = tail = nb;
        else { tail->next = nb; tail = nb; }
    }
    // free old list
    for (Block *p = head; p; ) {
        Block *n = p->next;
        free(p);
        p = n;
    }
    head = new_head;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <memory-size>\n", argv[0]);
        return EXIT_FAILURE;
    }
    total_mem = atoi(argv[1]);
    if (total_mem <= 0) {
        fprintf(stderr, "Invalid memory size\n");
        return EXIT_FAILURE;
    }
    // initial hole
    head = malloc(sizeof *head);
    head->start = 0;
    head->size  = total_mem; // mem amt passed to cmd
    head->free  = true;
    head->name[0] = '\0';
    head->next  = NULL;

    char line[CMD_LEN];
    while (1) {
        printf("allocator>");
        if (!fgets(line, CMD_LEN, stdin)) { // user input
            break;                          // exit loop
        }

        line[strcspn(line, "\n")] = '\0';  // delete newline on 'line'

        // request memory block with allocate_mem()
        if (strncmp(line, "RQ ", 3) == 0) {
            char pname[NAME_LEN]; 
            int sz; 
            char strat;
            if (sscanf(line, "RQ %31s %d %c", pname, &sz, &strat) == 3) {  // checks for correctly formatted RQ command
                allocate_mem(pname, sz, strat);
            }
            else
                printf("Error: Invalid RQ format\n");
        }

        // release memory block with process name P
        else if (strncmp(line, "RL ", 3) == 0) {
            char pname[NAME_LEN];
            if (sscanf(line+3, "%31s", pname) == 1) {
                release_mem(pname);
            }
            else {
                printf("Error: Invalid RL format\n");
            }
        }

        // compact memory
        else if (strcmp(line, "C") == 0) {
            compact_mem();
        }

        // show stats of memory blocks
        else if (strcmp(line, "STAT") == 0) {
            status();
        }

        // break out of loop (exit)
        else if (strcmp(line, "X") == 0) {
            break;
        }

        // else unknown command
        else if (line[0] != '\0') {
            printf("Error: Unknown command\n");
        }
    }

    // cleanup, free mem
    for (Block *p = head; p; ) {
        Block *n = p->next;
        free(p);
        p = n;
    }
    return 0;
}
