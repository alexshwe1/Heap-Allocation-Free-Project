////////////////////////////////////////////////////////////////////////////////
// Main File:        myHeap.c
// This File:        myHeap.c
// Other Files:      
// Semester:         CS 354 Fall 2020
// Instructor:       deppeler
// 
// Discussion Group: 642
// Author:           Alexander Shwe
// Email:            shwe@wisc.edu
// CS Login:         shwe
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          N/A
//
// Online sources:   Piazza
//
//////////////////////////// 80 columns wide ///////////////////////////////////
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           
    int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/* Reference to the payload of the current block in order to
 * support nextFit placement policy.
 */
blockHeader *currBlock = NULL;

/*
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding pad as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {

    //test for if payload size will fit in allocated size
    if(size <= 0 || size > allocsize) {
        return NULL;
    }

    int pad = 0;

    //figure out correct padding given size
    if((size + sizeof(blockHeader)) % 8 != 0) {
        pad = 8 - ((size + sizeof(blockHeader)) % 8);
    }

    //size of block to be inserted in the heap
    int insertBlockSize = size + sizeof(blockHeader) + pad;

    //test if this is the first time alloc is called in a given heap
    if(currBlock == NULL) {
        //set the current block to the start of the heap payload
        currBlock = (void*)heapStart + sizeof(blockHeader);
    }

    int tempFreeBlockHeader = ((blockHeader*)((void*)currBlock - sizeof(blockHeader)))->size_status;
    int tempFreeBlockSize = (tempFreeBlockHeader / 8) * 8;
    int clock = 0;

    //while loop to find the next free block
    while(1) {
        //test for if at the end block. If so, move to beginning of heap to follow nextFit placement policy
        if(((blockHeader*)((void*)currBlock - sizeof(blockHeader)))->size_status == 1) {
            currBlock = (void*)heapStart + sizeof(blockHeader);
	}
        //test if the current block is free
        if(tempFreeBlockHeader % 8 == 0 || tempFreeBlockHeader % 8 == 2) {
            //test if the size of the current block will fit intended payload
            if(insertBlockSize <= tempFreeBlockSize) {
                break;
            }
        }
        //increment current block
        currBlock = (blockHeader*)((void*)currBlock + tempFreeBlockSize);
        //increment clock
        clock += tempFreeBlockSize;
        tempFreeBlockHeader = ((blockHeader*)((void*)currBlock - sizeof(blockHeader)))->size_status;
        tempFreeBlockSize = (tempFreeBlockHeader / 8) * 8;
        //test if every block has been searched, if so, return NULL
        if(clock == allocsize) {
            return NULL;
        }
    }

    ((blockHeader*)((void*)currBlock - sizeof(blockHeader)))->size_status = insertBlockSize + 3;

    //check to see if split is needed
    if(!(insertBlockSize == tempFreeBlockSize)) {
        //new free block header for split block
        blockHeader *header = (blockHeader*) ((void*)currBlock - sizeof(blockHeader) + insertBlockSize);
        header->size_status = tempFreeBlockHeader - insertBlockSize;
        //test for if next block is end mark and update free block footer
        if(((blockHeader*)((void*)currBlock - sizeof(blockHeader) + tempFreeBlockSize - sizeof(blockHeader)*3))->size_status == 1) {
            ((blockHeader*)((void*)currBlock - sizeof(blockHeader) + tempFreeBlockSize - sizeof(blockHeader)*4))->size_status = tempFreeBlockSize - insertBlockSize;
        } else {
            ((blockHeader*)((void*)currBlock - sizeof(blockHeader) + tempFreeBlockSize - sizeof(blockHeader)*3))->size_status = tempFreeBlockSize - insertBlockSize;
        }
    }
    
    return currBlock;
}

/*
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */
int myFree(void *ptr) {

    //Return -1 if ptr is NULL.
    //Return -1 if ptr is not a multiple of 8.
    //Return -1 if ptr is outside of the heap space.
    //Return -1 if ptr block is already freed.
    if(ptr == NULL) {
        return -1;
    }
    if(((int)ptr) % 8 != 0) {
        return -1;
    }
    if(((int)ptr) < (int)heapStart || ((int)ptr) > (int)heapStart + allocsize) {
        return -1;
    }
    if(((blockHeader*)((void*)ptr - sizeof(blockHeader))) -> size_status % 2 == 0) {
        return -1;
    }

    int clock = 0;

    //variables for the current block
    int currBlockHeader = ((blockHeader*)((void*)ptr - sizeof(blockHeader)))->size_status;
    int currBlockSize = (currBlockHeader / 8) * 8;

    //variables for the next adjacent block
    blockHeader *nextBlock = (blockHeader*)((void*)ptr - sizeof(blockHeader) + currBlockSize);
    int nextAdjBlockHead = ((blockHeader*)((void*)nextBlock))->size_status;
    int nextAdjBlockSize = (nextAdjBlockHead / 8) * 8;

    //test if the next block is free, if so, coalesce
    if(nextAdjBlockHead % 8 == 0 || nextAdjBlockHead % 8 == 2) {
        //update a bit
        ((blockHeader*)((void*)ptr - sizeof(blockHeader)))->size_status -= 1;
        //update header
        ((blockHeader*)((void*)ptr - sizeof(blockHeader)))->size_status += nextAdjBlockSize;
        //update footer
        ((blockHeader*)((void*)ptr - sizeof(blockHeader)*2 + currBlockSize + nextAdjBlockSize))->size_status += currBlockSize;
        clock++;
    }

    currBlockHeader = ((blockHeader*)((void*)ptr - sizeof(blockHeader)))->size_status;
    currBlockSize = (currBlockHeader / 8) * 8;

    //test if previous adjacent block is free, if so, coalesce
    if(currBlockHeader % 8 == 0 || currBlockHeader % 8 == 1) {
        //local variable for the previous adjacent free block size
        int prevAdjBlockSize = ((blockHeader*)((void*)ptr - sizeof(blockHeader)*2))->size_status;
        //update previous adjacent free block header
        ((blockHeader*)((void*)ptr - sizeof(blockHeader) - prevAdjBlockSize))->size_status += currBlockSize;
        //create a free block footer
        blockHeader *footer = (blockHeader*)((void*)ptr - sizeof(blockHeader)*2 + currBlockSize);
        footer->size_status = currBlockSize + prevAdjBlockSize;
        //test for if next block is end mark, if not, update p bit of that next block
        if(((blockHeader*)((void*)ptr - sizeof(blockHeader) + currBlockSize))->size_status != 1) {
            ((blockHeader*)((void*)ptr - sizeof(blockHeader) + currBlockSize))->size_status -= 2;
        }

        currBlock = (blockHeader*)((void*)ptr - prevAdjBlockSize);
        clock++;
    }

    //test clock variable to see if no coalescing has occured
    if(clock == 0) {
        //update header and create footer
        ((blockHeader*)((void*)ptr - sizeof(blockHeader)))->size_status -= 1;
        blockHeader *footer = (blockHeader*)((void*)ptr - sizeof(blockHeader)*2 + currBlockSize);
        footer->size_status = currBlockSize;
        //test for if next block is end mark, if not, update p bit of that next block
        if(((blockHeader*)((void*)ptr - sizeof(blockHeader) + currBlockSize))->size_status != 1) {
            ((blockHeader*)((void*)ptr - sizeof(blockHeader) + currBlockSize))->size_status -= 2;
        }
    }

    return 0;

}
 
 
/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;

    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
} 


// end of myHeap.c (fall 2020)
