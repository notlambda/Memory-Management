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


static char *heap_listp;		// heap pointer
static size_t DW_SIZE = 16;     // double word size is equal 16
static size_t W_SIZE = 8;       // each word size is equal to 8
static size_t CHUNKSIZE = (1<<12);
/* **** HELPER FUNCTIONS ************************* */

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

static size_t GET (char *p)			// read word at address p
{
    // printf("p: %p\n", p);
	return (*(size_t *)(p));
}

static void PUT(char *p, size_t val)		// write word at address p
{
	(*(size_t *)(p)) = val;
}

static size_t GET_SIZE(char *p)		// read size at address p
{
	return (GET(p) & ~0xf);
}

static char *HDRP(char *bp)			// get address of header with block ptr
{
	return ((char *)(bp) - W_SIZE);
}

static char *FTRP(char *bp)			// get address of footer with block ptr
{
	return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DW_SIZE);
}

static char *NEXT_BLKP(char *bp)		// get address of next block using block ptr
{
	return (char *)(bp) + GET_SIZE(HDRP(bp));
}

static char *PREV_BLKP(char *bp)		// get address of previous blk using block ptr
{
	return (char *)(bp) - GET_SIZE((char *)bp - DW_SIZE);
}

static uint64_t GET_ALLOC(char *p)		// read allocated field from address p
{
	return (GET(p) & 0x1);
}

static int MAX (size_t x, size_t y) 
{
    return (x > y) ? x : y;             // if x is greater than y, returns x. else return y
}

static void *search_fit(size_t aligned_size)
{
    char *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0UL; bp = (char *)NEXT_BLKP(bp))
    {
        // size_t csize = GET_SIZE(HDRP(bp));
        if (!GET_ALLOC(HDRP(bp)) && (aligned_size <= GET_SIZE(HDRP(bp))))
        {
            // printf("csize: %lu\n", csize);
            return bp;
        }
    }
    return NULL;
}


static size_t PACK(size_t size, size_t alloc)
{
    return  ((size) | (alloc));
}

static void place(void *bp, size_t aligned_size)
{
    size_t csize = GET_SIZE(HDRP(bp));

    size_t remSize = csize - aligned_size; 
    // printf("cSize: %lu aligned_size: %lu remSize: %lu\n", csize, aligned_size, remSize);
    if (remSize >= (2 * DW_SIZE))
    {
        PUT(HDRP(bp), PACK(aligned_size, 1));
        PUT(FTRP(bp), PACK(aligned_size, 1));
        bp = NEXT_BLKP(bp);

        PUT(HDRP(bp), PACK(remSize, 0));
        PUT(FTRP(bp), PACK(remSize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* END OF HELPER FUNCTIONS */
/*****************************************************************************/
/* Merges adjacent free blocks together */
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // printf("prev block: %p and prev_alloc:%lu\n", PREV_BLKP(bp), prev_alloc);
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
        // printf("new size: %lu\n", size);
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
		size=(words+1)*W_SIZE;			// add 1 to make even and align to 4 byte word
	else						// to maintain alignment
		size=words*W_SIZE;				// otherwise just align to 4 byte word
    // printf("extend heap called size: %lu\n", size);
	if ((long)(bp=mem_sbrk(size)) == -1)		// if heap extension of size fails
		return NULL;				// :return NULL
    // printf("bp: %p size: %lu\n", (char *)bp, GET_SIZE(HDRP(bp)));
	PUT(HDRP(bp), (size|0));			// else: write size to header of blk ptr
	PUT(FTRP(bp), (size|0));			// 	 write size to footer of blk ptr
    // printf("size of bp: %lu", GET_SIZE(HDRP(bp)));
	PUT(HDRP(NEXT_BLKP(bp)), (0|1));		// write 1 to header of next blk to show allocation
	
	return coalesce(bp);				// coalesce any free blocks of newly extended heap
    // return bp;
}

/*
 * Initialize: returns false on error, true on success.
 */
// bool mm_init(void)
// {

//     	/* IMPLEMENT THIS */
//     	heap_listp = mem_sbrk(4 * W_SIZE);			// allocate 16 bytes and set returned ptr to heap ptr
//     	if ((heap_listp) == (void *)-1)			// check if failed
//     		return -1;				// return false if failed
//     	(*(size_t *)(heap_listp)) = 0;			// Alignment padding
//     	(*(size_t *)(heap_listp+4)) = (8|1);		// Prologue header
//     	(*(size_t *)(heap_listp+(8))) = (8|1);		// Prologue footer
//     	(*(size_t *)(heap_listp+(12))) = (0|1);		// Epilogue header
//     	heap_listp+=8;
    	
//     	if (extend_heap(1024) == NULL)			// extend empty heap with free block of 4096 bytes (1024 words)
//     		return false;				// if failed return false
//     	return true;					// otherwise return success
    	
// }

bool mm_init(void)
{
 /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*W_SIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*W_SIZE), PACK(DW_SIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*W_SIZE), PACK(DW_SIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*W_SIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*W_SIZE);
    // printf("heap_listp: %p\n", heap_listp);

 /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/W_SIZE) == NULL)
        return 0;
    return 1;
}



/*
 * malloc
 */
void* malloc(size_t size)
{
    size_t aligned_size = size; 
    size_t extend_size;
    char *bp;

    /* IMPLEMENT THIS */
    if (size == 0) {    // returns NULL if size is 0
        return NULL;
    }

    if (size <= DW_SIZE) {              // adjust block size and align the size
        aligned_size = 2 * DW_SIZE;
    }
    else {
        aligned_size = DW_SIZE + align(aligned_size);
    }

    // printf("aligned size: %lu\n", aligned_size);

    if ((bp = search_fit(aligned_size)) != NULL)  {
        // printf("bp size: %lu\n", GET_SIZE(HDRP(bp)));
        place(bp, aligned_size);
        // printf("alloc done for bp: %p\n", bp);
        return bp;
    }


    extend_size = aligned_size > CHUNKSIZE ? aligned_size : CHUNKSIZE;
    // printf("aligned_size: %lu extend_size: %lu\n", aligned_size, extend_size);
    if ((bp = extend_heap(extend_size/W_SIZE)) == NULL) {
        return NULL;
    }

    // printf("bp size: %lu\n", GET_SIZE(HDRP(bp)));
    place(bp, aligned_size);
    // printf("alloc done for bp: %p\n", bp);
    return bp;
}

/*
 * free
 */
void free(void* ptr)
{
    // printf("free called\n");
    /* IMPLEMENT THIS */
    if (ptr == NULL)
    {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));
    // printf("size: %lu\n", size);
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    // return ptr;

}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    // printf("realloc called\n");
    if (oldptr == NULL) {
        return malloc(size);
    }

    if (size == 0) {
        free(oldptr);
        return NULL;
    }

    void* new_ptr = malloc(size);       // malloc the new size,

    if (new_ptr == NULL) {          // if new_ptr equal null, then malloc failed, return null
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(oldptr));
    if (old_size > size) {     // Case 2: if old size is less than new size, 
        old_size = size;
    }

    memcpy(new_ptr, oldptr, old_size);     // copy data from oldptr to new ptr
    free(oldptr);                          // free data from the oldptr

    return new_ptr;                 // return ptr to newly allocated memory        
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
