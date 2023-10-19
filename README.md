# Checkpoint 1 Work
Andrew:
Implemented helper functions: GET, PUT, GET_SIZE, HDRP, FTRP, NEXT_BLKP, PREV_BLKP, and GET_ALLOC
Declared static char pointer variable heap_listp to point to the heap
Implemented coalesce function to join free blocks of memory
Implemented extend_heap function to increase the heap size if needed
Used coalesce and extend_heap in initialize function for heap
debugged each function for 100% correctness

Anh:
implemented the general idea for malloc, debugged
implemented helper functions: MAX, search_fit, PACK, and Place
Implemented realloc and free functions
debugged each function for 100% correctness

# Final Submission Work

Anh:
Updated the coalesce, place, search fit, init, free functions. 
Implemented the free delete and add functions into coalesce and place
Changed the search fit function to look through the freeptr list
Implemented the next/prev ptr into free function
Made the checkheap function
Debugged the code

Andrew:
Created next and previous pointer functions to return next/prev pointers of explicit free list
Created add_free and delete_free to add and delete from the free list
Initialized free list to start of memory in heap

