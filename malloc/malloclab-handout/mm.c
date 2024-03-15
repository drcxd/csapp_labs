/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "The A Team",
    /* First member's full name */
    "Chang Xiaoduan",
    /* First member's email address */
    "xiaoduan@bu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* #define NEXT_FIT */

#define PSIZE (sizeof(void**))
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val) (*(unsigned int *)(p) = (val))     // line:vm:mm:put
#define PUTP(p, val) (*(void**)(p) = (void**)(val))
/* get pointer to pointer to predecessor */
#define GETPP(p) ((void **)(p))
/* get pointer to pointer to successor */
#define GETNP(p) ((void**)(p) + 1)

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
static char *first_free = 0;
#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void unlink_blk(void *bp);
static void insert_front(void *bp);
static size_t round_size(size_t size);
static void *try_merge_realloc(void *bp, size_t size);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);
/* static void check_free_list(); */
static void report_heap();

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit
    first_free = NULL;
    /* $end mminit */

#ifdef NEXT_FIT
    rover = heap_listp;
#endif
    /* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    report_heap();
    return 0;
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  printf("allocating %d bytes\n", size);
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* $end mmmalloc */
    if (heap_listp == 0){
        mm_init();
    }
    /* $begin mmmalloc */
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    asize = round_size(size);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
        place(bp, asize);                  //line:vm:mm:findfitplace
        report_heap();
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
    printf("extending heap to allocate %d bytes\n", asize);
    report_heap();
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;                                  //line:vm:mm:growheap2
    place(bp, asize);                                 //line:vm:mm:growheap3
    report_heap();
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
  printf("freeing %p\n", bp);
    /* $end mmfree */
    if (bp == 0)
        return;

    /* $begin mmfree */
    size_t size = GET_SIZE(HDRP(bp));
    /* $end mmfree */
    if (heap_listp == 0){
        mm_init();
    }
    /* $begin mmfree */

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_front(bp);
    coalesce(bp);
    report_heap();
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  printf("reallocating %p\n", ptr);
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    oldsize = GET_SIZE(HDRP(ptr));

    if ((newptr = try_merge_realloc(ptr, size))) {
      report_heap();
      return newptr;
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    /* oldsize = GET_SIZE(HDRP(ptr)); */
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        return bp;
    }

    if (prev_alloc && !next_alloc) {      /* Case 2 */
      unlink_blk(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
      unlink_blk(bp);
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
      unlink_blk(NEXT_BLKP(bp));
      unlink_blk(bp);
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* $end mmfree */
#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp)))
        rover = bp;
#endif
    /* $begin mmfree */
    return bp;
}

void mm_checkheap(int verbose)
{
    checkheap(verbose);
}

static void *extend_heap(size_t words)
{

  /* { */
  /*   char *bp; */
  /*   for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) { */
  /*     if (!GET_ALLOC(HDRP(bp))) { */
  /*       coalesce(bp); */
  /*     } */
  /*   } */
  /* } */

    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;                                        //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr
    insert_front(bp);

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    unlink_blk(bp);

    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_front(bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static void *find_fit(size_t asize)
{

#ifdef NEXT_FIT
    /* Next fit search */
    char *oldrover = rover;

    /* Search from the rover to the end of list */
    for ( ; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
            return rover;

    /* search from start of list to old rover */
    for (rover = heap_listp; rover < oldrover; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
            return rover;

    return NULL;  /* no fit found */
#else
    /* $begin mmfirstfit */
    /* First-fit search */
    /* void *bp; */

    /* for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) { */
    /*     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) { */
    /*         return bp; */
    /*     } */
    /* } */
    /* return NULL; /\* No fit *\/ */

    void **it = (void**)first_free;
    void **best_fit = NULL;
    unsigned best_diff = UINT32_MAX;
    while (it != NULL) {
      unsigned int size = GET_SIZE(HDRP(it));
      if (asize <= size) {
        unsigned diff = size - asize;
        if (diff < best_diff) {
          best_diff = diff;
          best_fit = it;
        }
      }
      it = *(GETNP(it));
    }
    return best_fit;
#endif
}

static void printblock(void *bp)
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ud:%c] footer: [%ud:%c]\n", bp,
           hsize, (halloc ? 'a' : 'f'),
           fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp)
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

void checkheap(int verbose)
{
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}

void unlink_blk(void *bp) {
  void **prev_blk = *(GETPP(bp));
  void **next_blk = *(GETNP(bp));
  void **prev_next = prev_blk ? GETNP(prev_blk) : NULL;
  void **next_prev = next_blk ? GETPP(next_blk) : NULL;
  if (prev_next) {
    *prev_next = next_blk;
  }
  if (next_prev) {
    *next_prev = prev_blk;
  }
  if (bp == first_free) {
    first_free = (char *)next_blk;
  }
  /* check_free_list(); */
}

void insert_front(void *bp) {
  PUTP(GETPP(bp), 0);
  PUTP(GETNP(bp), first_free);
  if (first_free) {
    PUTP(GETPP(first_free), bp);
  }
  first_free = bp;
  /* check_free_list(); */
}

/* void check_free_list() { */
/*   void** it = (void**)first_free; */
/*   while (it) { */
/*     assert(!GET_ALLOC(HDRP(it))); */
/*     assert(it != *GETNP(it)); */
/*     it = *GETNP(it); */
/*   } */
/* } */

size_t round_size(size_t size) {
  if (size <= DSIZE) {
    return 2 * DSIZE; // line:vm:mm:sizeadjust2
  } // line:vm:mm:sizeadjust1
  return DSIZE *
         ((size + (1 * DSIZE) + (DSIZE - 1)) / DSIZE); // line:vm:mm:sizeadjust3
}

void *try_merge_realloc(void *bp, size_t size) {
  /* If next block is free and size is large enough to hold
     realloced block, then extend this block and skip the malloc and
     free */
  size_t oldsize = GET_SIZE(HDRP(bp));
  size_t asize = round_size(size);

  char *next_bp = NEXT_BLKP(bp);
  char next_alloc = GET_ALLOC(HDRP(next_bp));
  size_t next_size = GET_SIZE(HDRP(next_bp));

  char *prev_bp = PREV_BLKP(bp);
  char prev_alloc = GET_ALLOC(HDRP(prev_bp));
  size_t prev_size = GET_SIZE(HDRP(prev_bp));

  if (prev_alloc && next_alloc) {
    return NULL;
  }

  if (prev_alloc && !next_alloc && (next_size + oldsize >= asize)) {
    size_t total_size = next_size + oldsize;
    unlink_blk(next_bp);
    size_t remain_size = total_size - asize;
    if (remain_size >= 2 * DSIZE) {
      PUT(HDRP(bp), PACK(asize, 1));
      PUT(FTRP(bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(new_next_bp);
    } else {
      PUT(HDRP(bp), PACK(total_size, 1));
      PUT(FTRP(bp), PACK(total_size, 1));
    }
    return bp;

  } else if (!prev_alloc && next_alloc && (prev_size + oldsize >= asize)) {
    size_t total_size = prev_size + oldsize;
    size_t remain_size = total_size - asize;
    unlink_blk(prev_bp);
    if (size < oldsize) {
      oldsize = size;
    }
    for (int i = 0; i < oldsize; ++i) {
      *(prev_bp + i) = *((char*)bp + i);
    }
    if (remain_size >= 2 * DSIZE) {
      PUT(HDRP(prev_bp), PACK(asize, 1));
      PUT(FTRP(prev_bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(prev_bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(new_next_bp);
    } else {
      PUT(HDRP(prev_bp), PACK(total_size, 1));
      PUT(FTRP(prev_bp), PACK(total_size, 1));
    }
    return prev_bp;
  } else if (!prev_alloc && !next_alloc &&
             (prev_size + next_size + oldsize >= asize)) {
    size_t total_size = prev_size + next_size + oldsize;
    size_t remain_size = total_size - asize;
    unlink_blk(prev_bp);
    unlink_blk(next_bp);
    if (size < oldsize) {
      oldsize = size;
    }
    for (int i = 0; i < oldsize; ++i) {
      *(prev_bp + i) = *((char*)bp + i);
    }
    if (remain_size >= 2*DSIZE) {
      PUT(HDRP(prev_bp), PACK(asize, 1));
      PUT(FTRP(prev_bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(prev_bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(new_next_bp);
    } else {
      PUT(HDRP(prev_bp), PACK(total_size, 1));
      PUT(FTRP(prev_bp), PACK(total_size, 1));
    }
    return prev_bp;
  }
  return NULL;
}

static void report_heap() {
  printf("reporting heap...\n");
  char *bp;
  int i = 0;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    printf("block: %d\taddress: %p\tsize: %d\tallocated:%d\n", i, bp, GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    ++i;
  }
}
