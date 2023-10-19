/*
 * mm.c
 *
 * Name: [Anh Nguyen & Andrew Latini]
 * Program Description:
 *      For this project we implemented a dynamic storage allocator in C.
 *      We created our own malloc, realloc, and free fuctions, using any 
 *      helper functions as necessary. The heap is initialized as an empty
 *      heap of free blocks. 
 *      Malloc is an application that requests a block of size bytes.
 *      Upon checking for invalid requests, the requested block size
 *      is adjusted to allow room for the header and footer, while 
 *      satisfying the double-word alignment. A minimum block size of
 *      16 is enforced, 8 bytes for the alignment and another 8 for the
 *      header and footer. Any request greater than 8 bytes is added with
 *      the header/footer bytes and rounded to the nearest multiple of 8.
 *      Once it has adjusted to the correct size, it searches for the free 
 *      list for a suitable free block. If there is a fit, the block is
 *      placed and any excess is split. Finally, the address of the new 
 *      block is returned. If there is no fit, the heap is extended with 
 *      a new free block and places the requested block in that free block.
 * We reference our work from the textbook.
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
static char *freePtr = 0;		// free list ptr = start of explicit list
static size_t DW_SIZE = 16;     	// double word size is equal 16
static size_t W_SIZE = 8;       	// each word size is equal to 8
static size_t CHUNKSIZE = (1<<12);	// chunk size = 4kb
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

static char **NEXT_PTR(char *bp)
{
	return ((char **)(bp+W_SIZE));		// point to next ptr of free list
}

static char **PREV_PTR(char *bp)
{
	return ((char **)(bp));			// point to previous ptr of free list
}

static void free_add(char *bp)
{
    dbg_printf("free_add: %p freePtr: %p\n", bp, freePtr);
	char **nextPtr = NEXT_PTR(bp);		// gets next ptr of new free
	*nextPtr = freePtr;			// sets next ptr to the current free blk
	
	if(freePtr)				
	{
		char **prevFPtr = PREV_PTR(freePtr);	// gets previous pointer of current free blk
		*prevFPtr = bp;				// sets previous ptr to the new free
	}
	
	char **prevPtr = PREV_PTR(bp);			// get previous ptr of new free
	*prevPtr = NULL;				// set it to NULL
	freePtr = bp;					// set current block to new free
    dbg_printf("freePtr: %p\n", freePtr);
    mm_checkheap(0);
}

static void free_delete(char *ptr)
{
    dbg_printf("deleting: %p\n", ptr);
	if (*PREV_PTR(ptr) == NULL)				// if first in list
	{
		freePtr = *NEXT_PTR(ptr);			// set current free to next address of deleted block
        dbg_printf("freePtr: %p\n", ptr);	
	}
	else
	{
		char **nextPtr = NEXT_PTR(*PREV_PTR(ptr));	// get next pointer of previous block of deleted ptr
		*nextPtr = *NEXT_PTR(ptr);			// set to next ptr of deleted block
	}
	
	if (*NEXT_PTR(ptr))					
	{
		char **prevPtr = PREV_PTR(*NEXT_PTR(ptr));	// get previous ptr of next block of deleted ptr
		*prevPtr = *PREV_PTR(ptr);			// set to previous ptr of deleted block
	}	
}

static int MAX (size_t x, size_t y) 
{
    return (x > y) ? x : y;             // if x is greater than y, returns x. else return y
}

static void *search_fit(size_t aligned_size)
{
    char *bp;

    for (bp = freePtr; bp; bp = *NEXT_PTR(bp))		// loop through free list to find a fit
    {
        if (aligned_size <= GET_SIZE(HDRP(bp)))		// if fit found return a pointer to that block
        {
            return bp;
        }
    }
    return NULL;									// otherwise return NULL
}


static size_t PACK(size_t size, size_t alloc)
{
    return  ((size) | (alloc));
}

static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    dbg_printf("prev block: %p and prev_alloc:%lu next block: %p and next_alloc: %lu\n", PREV_BLKP(bp), prev_alloc, NEXT_BLKP(bp), next_alloc);
	size_t size = GET_SIZE(HDRP(bp));
	
	if (prev_alloc && next_alloc) {			// Case 1: if adjacent blocks are both allocated,
		free_add(bp);                       // add block to free list
        return bp;				            // return the block ptr
    }

	else if (prev_alloc && !next_alloc) 		// Case 2: if prev block is allocated but next blk is free, 
	{										
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp))); 	// add size of free blk header to size
        free_delete(NEXT_BLKP(bp));             // delete previous block from free list
		PUT(HDRP(bp), (size|0));		// write size to blk ptr header and footer
		PUT(FTRP(bp), (size|0));		
	}
	
	else if (!prev_alloc && next_alloc)		// Case 3: if prev blk free but next blk is allocated
	{
		size+=GET_SIZE(HDRP(PREV_BLKP(bp)));	// add header of free block size to size
        dbg_printf("new size: %lu\n", size);
        dbg_printf("prev blkp: %p\n", PREV_BLKP(bp));
        free_delete(PREV_BLKP(bp));             // delete previous block from free list
		PUT(FTRP(bp), (size|0));		// write size to footer
		PUT(HDRP(PREV_BLKP(bp)), (size|0));	// write size to free blk's header
		bp = PREV_BLKP(bp);			// set blk ptr to the free blk
	}
	
	else {										// Case 4: If both blocks free
		size+=GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));	// add size of previous blk header and next blk footer to size
        free_delete(PREV_BLKP(bp));                         // delete previous block from free list
        free_delete(NEXT_BLKP(bp));                         // delete next block from free list
		PUT(HDRP(PREV_BLKP(bp)), (size|0));					// write size to previous blk header
		PUT(FTRP(NEXT_BLKP(bp)), (size|0));					// write size to next blk footer
		bp = PREV_BLKP(bp);							// set blk ptr to previous blk
	}
    free_add(bp);               // add blk ptr to free list
	return bp;					// return the blk ptr
}

static void place(void *bp, size_t aligned_size)
{
    size_t csize = GET_SIZE(HDRP(bp));

    size_t remSize = csize - aligned_size; 
    dbg_printf("cSize: %lu aligned_size: %lu remSize: %lu\n", csize, aligned_size, remSize);
    if (remSize >= (2 * DW_SIZE)) 		// splitting the block
    {
        PUT(HDRP(bp), PACK(aligned_size, 1));		// takes in size and OR with 1, and store 
        PUT(FTRP(bp), PACK(aligned_size, 1));
        free_delete(bp);                // delete the free block at address bp
        bp = NEXT_BLKP(bp);				// allocate the first half and freeing the second half

        PUT(HDRP(bp), PACK(remSize, 0));		// put remSize in header and footer
        PUT(FTRP(bp), PACK(remSize, 0));
        coalesce(bp);

    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));			// otherwise put csize in header and footer
        PUT(FTRP(bp), PACK(csize, 1));
        free_delete(bp);                        // delete the free block at address bp
    }
}

/* END OF HELPER FUNCTIONS */
/*****************************************************************************/
/* Merges adjacent free blocks together */

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
    dbg_printf("extend heap called size: %lu\n", size);
	if ((long)(bp=mem_sbrk(size)) == -1)		// if heap extension of size fails
		return NULL;				// :return NULL
    dbg_printf("bp: %p size: %lu\n", (char *)bp, GET_SIZE(HDRP(bp)));
	PUT(HDRP(bp), (size|0));			// else: write size to header of blk ptr
	PUT(FTRP(bp), (size|0));			// 	 write size to footer of blk ptr
    dbg_printf("size of bp: %lu", GET_SIZE(HDRP(bp)));
	PUT(HDRP(NEXT_BLKP(bp)), (0|1));		// write 1 to header of next blk to show allocation
	
	return coalesce(bp);				// coalesce any free blocks of newly extended heap
    // return bp;
}

/*
 * Initialize: returns false on error, true on success.
 */

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

    freePtr = NULL;         // initialize free list to start of free memory in heap
	 /* Extend the empty heap with a free block of CHUNKSIZE bytes */
	    if (extend_heap(CHUNKSIZE/W_SIZE) == NULL)
        return 0;
    mm_checkheap(0);
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

    dbg_printf("aligned size: %lu\n", aligned_size);

    if ((bp = search_fit(aligned_size)) != NULL)  {		// check if there is a fit
        dbg_printf("bp size: %lu\n", GET_SIZE(HDRP(bp)));
        place(bp, aligned_size);
        mm_checkheap(0);
        dbg_printf("alloc done for bp: %p\n", bp);
        return bp;
    }


    extend_size = aligned_size > CHUNKSIZE ? aligned_size : CHUNKSIZE;			// otherwise heap must be extended
    dbg_printf("aligned_size: %lu extend_size: %lu\n", aligned_size, extend_size);
    if ((bp = extend_heap(extend_size/W_SIZE)) == NULL) {				// extend heap
        return NULL;									               // if fail return NULL
    }

    mm_checkheap(0);
    dbg_printf("bp size: %lu\n", GET_SIZE(HDRP(bp)));
    place(bp, aligned_size);								// otherwise place in free block
    mm_checkheap(0);
    dbg_printf("alloc done for bp: %p\n", bp);
    return bp;
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    if (ptr == NULL)
    {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));
    dbg_printf("free called for %p of size: %lu\n", (char *)ptr, size);
    dbg_printf("size: %lu\n", size);
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    *NEXT_PTR(ptr) = NULL;
    *PREV_PTR(ptr) = NULL;
    coalesce(ptr);
    mm_checkheap(0);
    // return ptr;

}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    dbg_printf("realloc called\n");
    if (oldptr == NULL) {
        return malloc(size);
    }

    if (size == 0) {
        free(oldptr);
        return NULL;
    }

    char* new_ptr = malloc(size);       // malloc the new size,

    if (new_ptr == NULL) {          // if new_ptr equal null, then malloc failed, return null
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(oldptr));           // set old size to size of old ptr     
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
    char* bp;
    
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp))          // loops through the heap while the header is not equal to zero
    {
        if (GET_ALLOC(HDRP(bp)))                                                // checks if the block is allocated   
        {
            printf("Allocated Block: %p of size: %lu/n", bp, GET_SIZE(HDRP(bp)));       // prints the address of the block and the size of the block
        }
        else 
        {
            printf("Free Block: %p of size: %lu\n", bp, GET_SIZE(HDRP(bp)));        // otherwise its a free, so it will print the address and the size of the free block
        }
    }

    printf("-------\n");
    for (bp = freePtr; bp; bp = *NEXT_PTR(bp))                              // loops through the free list 
    {
        printf("Free block (explicit list): %p of size: %lu\n", bp, GET_SIZE(HDRP(bp)));        // prints the address of the free block and the size
    }
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
#endif /* DEBUG */
    return true;
}
