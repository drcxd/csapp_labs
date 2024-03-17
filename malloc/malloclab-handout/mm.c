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
#define SMALL_BLOCK_CHUNKSIZE (1 << 12)

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
static void *big_free = 0;
static char *epilogue_header = 0;
#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void* place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void unlink_blk(void **headp, void *bp);
static void insert_front(void **headp, void *bp);
static size_t round_size(size_t size);
static void *try_merge_realloc(void *bp, size_t size);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);
/* static void check_free_list(); */
static void report_heap();
static char is_small_block(void *bp);
static void *try_coalesce_with_prev(void *bp);
static void *try_coalesce_with_next(void *bp);
static void **get_list_by_size(size_t size);
static void **partition_block_by_size(void *bp, size_t size);
static void **get_last_block();

/* #define DBGLG */
#ifdef DBGLG
#define PRTF(fmt, ...) (printf(fmt, __VA_ARGS__))
#else
#define PRTF(fmt, ...)
#endif

/* #define SMALL_BLOCK_SIZE 256 */
#define SMALL_BLOCK_SIZE 112
#define SMALL_LIST_SIZE 31 /* 256/8 - 1*/
void **small_lists = NULL;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(34*WSIZE)) == (void *)-1) /* 31 + 3 */
        return -1;
  small_lists = (void**)heap_listp;
  bzero(heap_listp, SMALL_LIST_SIZE * PSIZE);
  heap_listp += SMALL_LIST_SIZE * PSIZE - PSIZE;
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit
    big_free = NULL;
    epilogue_header = heap_listp + WSIZE;
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
    PRTF("allocating %d bytes\n", asize);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
        /* place(bp, asize);                  //line:vm:mm:findfitplace */
        /* return bp; */
      return place(bp, asize);
    }

    /* if small block
         try allocate from big_free
         extend heap
       else if big
         extend heap
    */


    if (asize <= SMALL_BLOCK_SIZE) {
      bp = find_fit(SMALL_BLOCK_CHUNKSIZE);
      if (!bp) {
        if ((bp = extend_heap(SMALL_BLOCK_CHUNKSIZE/WSIZE)) == NULL) {
          return NULL;
        }
      }
      bp = (char*)partition_block_by_size(bp, asize);
    } else {
      extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
      char *last_block = (char *)get_last_block();
      if (!GET_ALLOC(HDRP(last_block)) && !is_small_block(last_block)) {
        extendsize -= GET_SIZE(HDRP(last_block));
      }
      if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL; // line:vm:mm:growheap2
    }
    return place(bp, asize);
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
  PRTF("freeing %p\n", bp);
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
    void **list = get_list_by_size(size);
    insert_front(list, bp);
    coalesce(bp);
    report_heap();
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  PRTF("reallocating %p to size %d\n", ptr, round_size(size));
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

    if (oldsize > SMALL_BLOCK_SIZE && size > SMALL_BLOCK_SIZE)  {
      if ((newptr = try_merge_realloc(ptr, size))) {
        report_heap();
        return newptr;
      }
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
  if (is_small_block(bp)) {
    return bp;
  }
  bp = try_coalesce_with_next(bp);
  bp = try_coalesce_with_prev(bp);
  return bp;
}

void mm_checkheap(int verbose)
{
    checkheap(verbose);
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;                                        //line:vm:mm:endextend

    char* last_block = (char*)get_last_block();
    if (!GET_ALLOC(HDRP(last_block)) && !is_small_block(last_block)) {
      size_t oldsize = GET_SIZE(HDRP(last_block));
      PUT(HDRP(last_block), PACK(size + oldsize, 0));
      PUT(FTRP(last_block), PACK(size + oldsize, 0));
      PUT(HDRP(NEXT_BLKP(last_block)), PACK(0, 1));
      epilogue_header = NEXT_BLKP(last_block) - 4;
      bp = last_block;
    } else {
      /* Initialize free block header/footer and the epilogue header */
      PUT(HDRP(bp), PACK(size, 0));
          /* Free block header */ // line:vm:mm:freeblockhdr
      PUT(FTRP(bp), PACK(size, 0));
          /* Free block footer */ // line:vm:mm:freeblockftr
      PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
          /* New epilogue header */ // line:vm:mm:newepihdr
      epilogue_header = NEXT_BLKP(bp) - 4;
      insert_front(&big_free, bp);
      bp = coalesce(bp);
    }


    /* Coalesce if the previous block was free */
    return bp;
}

static void *place(void *bp, size_t asize) {
  assert(!GET_ALLOC(HDRP(bp)));
    size_t csize = GET_SIZE(HDRP(bp));
    void **list = get_list_by_size(csize);
    unlink_blk(list, bp);

    if (asize <= SMALL_BLOCK_SIZE) {
      assert(asize == csize);
      PUT(HDRP(bp), PACK(asize, 1));
      PUT(FTRP(bp), PACK(asize, 1));
    } else {
      if ((csize - asize) > SMALL_BLOCK_SIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_front(&big_free, bp);
        bp = PREV_BLKP(bp);
      } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
      }
    }
    report_heap();
    return bp;
}

static void *find_fit(size_t asize)
{
    void** it;
    if (asize <= SMALL_BLOCK_SIZE) {
      it = get_list_by_size(asize);
      if (*it != NULL) {
        assert(!GET_ALLOC(HDRP(*it)));
        return *it;
      }
      return NULL;
    }

    it = big_free;
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

void unlink_blk(void** headp, void *bp) {
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
  if (bp == *headp) {
    *headp = next_blk;
  }
  /* check_free_list(); */
}

void insert_front(void **headp, void *bp) {
  PUTP(GETPP(bp), 0);
  PUTP(GETNP(bp), *headp);
  if (*headp) {
    PUTP(GETPP(*headp), bp);
  }
  *headp = bp;
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

#define SPLIT_RATIO 2
void *try_merge_realloc(void *bp, size_t size) {
  /* If next block is free and size is large enough to hold
     realloced block, then extend this block and skip the malloc and
     free */
  size_t oldsize = GET_SIZE(HDRP(bp));
  size_t asize = round_size(size);

  if (asize <= oldsize) {
    return bp;
  }

  /* if (asize < oldsize) { */
  /*   size_t remain_size = oldsize - asize; */
  /*   /\* if ((oldsize /remain_size) <= SPLIT_RATIO && remain_size >= 2*DSIZE) { *\/ */
  /*   if (remain_size >= SMALL_BLOCK_SIZE) { */
  /*     PUT(HDRP(bp), PACK(asize, 1)); */
  /*     PUT(FTRP(bp), PACK(asize, 1)); */
  /*     char *new_next_bp = NEXT_BLKP(bp); */
  /*     PUT(HDRP(new_next_bp), PACK(remain_size, 0)); */
  /*     PUT(FTRP(new_next_bp), PACK(remain_size, 0)); */
  /*     insert_front(&big_free, new_next_bp); */
  /*     coalesce(new_next_bp); */
  /*   } else { */
  /*     PUT(HDRP(bp), PACK(oldsize, 1)); */
  /*     PUT(FTRP(bp), PACK(oldsize, 1)); */
  /*   } */
  /*   return bp; */
  /* } */

  char *next_bp = NEXT_BLKP(bp);
  char next_alloc = GET_ALLOC(HDRP(next_bp));
  size_t next_size = GET_SIZE(HDRP(next_bp));

  char *prev_bp = PREV_BLKP(bp);
  char prev_alloc = GET_ALLOC(HDRP(prev_bp));
  size_t prev_size = GET_SIZE(HDRP(prev_bp));

  if (!next_alloc && (next_size + oldsize >= asize) && !is_small_block(next_bp)) {
    size_t total_size = next_size + oldsize;
    unlink_blk(&big_free, next_bp);
    size_t remain_size = total_size - asize;
    if ((total_size / remain_size) <= SPLIT_RATIO && remain_size >= SMALL_BLOCK_SIZE) {
    /* if (remain_size >= SMALL_BLOCK_SIZE) { */
      PUT(HDRP(bp), PACK(asize, 1));
      PUT(FTRP(bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(&big_free, new_next_bp);
    } else {
      PUT(HDRP(bp), PACK(total_size, 1));
      PUT(FTRP(bp), PACK(total_size, 1));
    }
    return bp;
  }

  if (!prev_alloc && (prev_size + oldsize >= asize) && !is_small_block(prev_bp)) {
    size_t total_size = prev_size + oldsize;
    size_t remain_size = total_size - asize;
    unlink_blk(&big_free, prev_bp);
    if (size < oldsize) {
      oldsize = size;
    }
    for (int i = 0; i < oldsize; ++i) {
      *(prev_bp + i) = *((char*)bp + i);
    }
    if ((total_size / remain_size) <= SPLIT_RATIO && remain_size >= SMALL_BLOCK_SIZE) {
    /* if (remain_size >= SMALL_BLOCK_SIZE) { */
      PUT(HDRP(prev_bp), PACK(asize, 1));
      PUT(FTRP(prev_bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(prev_bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(&big_free, new_next_bp);
    } else {
      PUT(HDRP(prev_bp), PACK(total_size, 1));
      PUT(FTRP(prev_bp), PACK(total_size, 1));
    }
    return prev_bp;
  }

  if (!prev_alloc && !next_alloc &&
      (prev_size + next_size + oldsize >= asize)) {
    return NULL;
    size_t total_size = prev_size + next_size + oldsize;
    size_t remain_size = total_size - asize;
    unlink_blk(&big_free, prev_bp);
    unlink_blk(&big_free, next_bp);
    if (size < oldsize) {
      oldsize = size;
    }
    for (int i = 0; i < oldsize; ++i) {
      *(prev_bp + i) = *((char*)bp + i);
    }
    /* if (remain_size >= 2*DSIZE) { */
    if ((total_size / remain_size) <= SPLIT_RATIO && remain_size >= 2*DSIZE) {
      PUT(HDRP(prev_bp), PACK(asize, 1));
      PUT(FTRP(prev_bp), PACK(asize, 1));
      char *new_next_bp = NEXT_BLKP(prev_bp);
      PUT(HDRP(new_next_bp), PACK(remain_size, 0));
      PUT(FTRP(new_next_bp), PACK(remain_size, 0));
      insert_front(&big_free, new_next_bp);
    } else {
      PUT(HDRP(prev_bp), PACK(total_size, 1));
      PUT(FTRP(prev_bp), PACK(total_size, 1));
    }
    return prev_bp;
  }


  void **last_block = get_last_block();
  if (last_block == bp) {
    /* extend by 1 page rather than allocate a whole new block */
    if ((long)mem_sbrk(CHUNKSIZE) == -1) {
      return NULL;
    }
    PUT(HDRP(bp), PACK(oldsize + CHUNKSIZE, 1));
    PUT(FTRP(bp), PACK(oldsize + CHUNKSIZE, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    epilogue_header = NEXT_BLKP(bp) - 4;
    return bp;
  }

  return NULL;
}

void report_heap() {
#ifdef DBGLG
  printf("reporting heap...\n");
  char *bp;
  int i = 0;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (!is_small_block(bp)) {
      printf("block: %d\taddress: %p\tsize: %d\tallocated:%d\n", i, bp, GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    }
    ++i;
  }
#endif
}

char is_small_block(void *bp) {
  return GET_SIZE(HDRP(bp)) <= SMALL_BLOCK_SIZE;
  /* return 0; */
}

void *try_coalesce_with_prev(void *bp) {
  if (is_small_block(bp)) {
    return bp;
  }
  void *prev = PREV_BLKP(bp);
  size_t size = GET_SIZE(HDRP(bp));
  if (!GET_ALLOC(HDRP(prev)) && !is_small_block(prev)) {
    unlink_blk(&big_free, bp);
    size_t prev_size = GET_SIZE(HDRP(prev));
    size += prev_size;
    PUT(HDRP(prev), PACK(size, 0));
    PUT(FTRP(prev), PACK(size, 0));
    bp = prev;
  }
  return bp;
}

void *try_coalesce_with_next(void *bp) {
  if (is_small_block(bp)) {
    return bp;
  }
  void *next = NEXT_BLKP(bp);
  size_t size = GET_SIZE(HDRP(bp));
  if (!GET_ALLOC(HDRP(next)) && !is_small_block(next)) {
    unlink_blk(&big_free, next);
    size_t next_size = GET_SIZE(HDRP(next));
    size += next_size;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  return bp;
}

void **get_list_by_size(size_t size) {
  if (size <= SMALL_BLOCK_SIZE) {
    return small_lists + (size / 8) - 2;
  }
  return &big_free;
}

void **partition_block_by_size(void *bp, size_t size) {
  size_t bsize = GET_SIZE(HDRP(bp));
  assert(bsize >= SMALL_BLOCK_SIZE);
  unlink_blk(&big_free, bp);

  void **list = get_list_by_size(size);
  void **to_return = NULL;
  while (bsize >= size && bsize - size >= 2*DSIZE) {
    /* partition the block */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_front(list, bp);
    void *next = NEXT_BLKP(bp);
    PUT(HDRP(next), PACK(bsize - size, 0));
    PUT(FTRP(next), PACK(bsize - size, 0));
    bsize -= size;
    bp = next;
  }
  to_return = *list;
  if (bsize > 0) {
    assert(bsize >= 2*DSIZE);
    assert(GET_SIZE(HDRP(bp)) == bsize);
    list = get_list_by_size(bsize);
    insert_front(list, bp);
  }
  assert(to_return);
  return to_return;
}

void **get_last_block() {
  return (void**)(epilogue_header - GET_SIZE(epilogue_header - WSIZE) + WSIZE);
}
