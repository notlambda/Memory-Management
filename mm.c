/*
 * mm.c
 *
 * Name: Andrew Latini
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

static char *heap_listp;		// heap pointer

/* **** HELPER FUNCTIONS ************************* */
/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

static size_t GET (char *p)			// read word at address p
{
	return (*(size_t *)(p));
}

static void PUT(char *p, size_t val)		// write word at address p
{
	(*(size_t *)(p)) = val;
}

static uint64_t GET_SIZE(char *p)		// read size at address p
{
	return (uint64_t)GET(p) & ~0x7;
}

static char *HDRP(char *bp)			// get address of header with block ptr
{
	return ((char *)(bp) - 4);
}

static char *FTRP(char *bp)			// get address of footer with block ptr
{
	return ((char *)(bp) + GET_SIZE(HDRP(bp)));
}

static char *NEXT_BLKP(char *bp)		// get address of next block using block ptr
{
	return ((char *)(bp) + GET_SIZE(HDRP(bp)) + ALIGNMENT);
}

static char *PREV_BLKP(char *bp)		// get address of previous blk using block ptr
{
	return ((char *)(bp) - (GET_SIZE(((char *)(bp) - ALIGNMENT)) + ALIGNMENT));
}

static uint64_t GET_ALLOC(char *p)		// read allocated field from address p
{
	return (uint64_t)GET(p) & 0x1;
}

/* END OF HELPER FUNCTIONS */
/*****************************************************************************/
/* Merges adjacent free blocks together */
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	
	if (prev_alloc && next_alloc)			// Case 1: if adjacent blocks are both
		return bp;				// allocated, return the block ptr
		
	else if (prev_alloc && !next_alloc) 		// Case 2: if prev block is allocated but next blk is free, 
	{										
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp))); 	// add size of free blk header to size
		PUT(HDRP(bp), (size|0));		// write size to blk ptr header and footer
		PUT(FTRP(bp), (size|0));		
	}
	
	else if (!prev_alloc && next_alloc)		// Case 3: if prev blk free but next blk is allocated
	{
		size+=GET_SIZE(HDRP(PREV_BLKP(bp)));	// add header of free block size to size
		PUT(FTRP(bp), (size|0));		// write size to footer
		PUT(HDRP(PREV_BLKP(bp)), (size|0));	// write size to free blk's header
		bp = PREV_BLKP(bp);			// set blk ptr to the free blk
	}
	
	else {										// Case 4: If both blocks free
		size+=GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));	// add size of previous blk header and next blk footer to size
		PUT(HDRP(PREV_BLKP(bp)), (size|0));					// write size to previous blk header
		PUT(FTRP(NEXT_BLKP(bp)), (size|0));					// write size to next blk footer
		bp = PREV_BLKP(bp);							// set blk ptr to previous blk
	}
	return bp;					// return the blk ptr
}
/*
*  extends heap by size 'words'
*/
static void *extend_heap(size_t words)
{
	char *bp;					// block pointer
	size_t size;
	
	if (words%2)					// if odd
		size=(words+1)*4;			// add 1 to make even and align to 4 byte word
	else						// to maintain alignment
		size=words*4;				// otherwise just align to 4 byte word
	if ((long)(bp=mem_sbrk(size)) == -1)		// if heap extension of size fails
		return NULL;				// :return NULL
	PUT(HDRP(bp), (size|0));			// else: write size to header of blk ptr
	PUT(FTRP(bp), (size|0));			// 	 write size to footer of blk ptr
	PUT(HDRP(NEXT_BLKP(bp)), (0|1));		// write 1 to header of next blk to show allocation
	
	return coalesce(bp);				// coalesce any free blocks of newly extended heap
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    	/* IMPLEMENT THIS */
    	heap_listp = mem_sbrk(16);			// allocate 16 bytes and set returned ptr to heap ptr
    	if ((heap_listp) == (void *)-1)			// check if failed
    		return false;				// return false if failed
    	(*(size_t *)(heap_listp)) = 0;			// Alignment padding
    	(*(size_t *)(heap_listp+4)) = (8|1);		// Prologue header
    	(*(size_t *)(heap_listp+(8))) = (8|1);		// Prologue footer
    	(*(size_t *)(heap_listp+(12))) = (0|1);		// Epilogue header
    	heap_listp+=8;
    	
    	if (extend_heap(1024) == NULL)			// extend empty heap with free block of 4096 bytes (1024 words)
    		return false;				// if failed return false
    	return true;					// otherwise return success
    	
}


/*
 * malloc
 */
void* malloc(size_t size)
{
	return NULL;
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    return NULL;
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
