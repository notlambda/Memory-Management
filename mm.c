/*
 * mm.c
 *
 * Name: [Anh Nguyen & Andrew Latini]
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read the project PDF document carefully and in its entirety before beginning.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment? */
#define ALIGNMENT 16

typedef struct block {
    size_t size;
    uint64_t *head;
    struct block* next;
}block_t;

block_t* head_block;


/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */
    head_block = NULL;

    return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    uint64_t aligned_size; 
    /* IMPLEMENT THIS */
    if (size == 0) {    // returns NULL if size is 0
        return NULL;
    }

    void *i;                                    // the current pointer use for the for loop
    uint64_t tracker = 0;                    // tracks number of consecutive free blocks in the loop
    void *head = mem_heap_lo();                   //  the starting address of the heap

    if (size > 16) {                                 // aligned size needs to be 16, so if not, the function help allign the size
        aligned_size = align(size);
    } else {                                        // else we assign it 16
        aligned_size = 16;
    }
    
    for (i = mem_heap_lo(); i <= mem_heap_hi(); i++) {  // a for loop to look into the heap
        if (tracker == aligned_size) {                  // check if tracker reach the number of blocks we need, it returns the head pointer
            static block_t block;
            block.size = aligned_size;
            block.head = head;
            block.next = NULL;                         
            if (head_block == NULL) {                   // creates a block
                head_block = &block;
                }else {
                    head_block->next = &block;
                }
            }
            return head;
        }
        
        if (((aligned_size & 1) % 2) == 1) {    // is the least significant bit is odd, we know its being used
                                               // hence we reset tracker
            tracker = 0;
        } else {                                // else it iterates the tracker, and reset head if tracker returns zero 
            if (tracker == 0) {
                head = i;
            }
            tracker += 1;
        }

        uint64_t *result = mem_sbrk(aligned_size);    
        if (result == (void *) -1) {                    // if we couldnt find the block in the heap, 
                                                        // we resort to mem_sbrk to add more memory to the heap
                                                        // and if mem_sbrk fail, we return NULL
            return NULL;
        } else{
            static block_t block;
            block.size = aligned_size;
            block.head = head;
            block.next = NULL;                         // recreates the block after adding more memory
            if (head_block == NULL) {
                head_block = &block;
                }else {
                    head_block->next = &block;
                }
            return result;
        }
    return NULL;
    }

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */

    return;
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    return NULL;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
#endif /* DEBUG */
    return true;
}
