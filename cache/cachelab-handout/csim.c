#include "cachelab.h"

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
/* #include <getopt.h> */

extern char *optarg;
extern int optind, opterr, optopt;

typedef struct cache {
  unsigned char *data;
  int s;
  int E;
  int b;
  int t;
  int line_size;
  int set_size;
  int cache_size;
} cache_t;

typedef struct op {
  char type;
  int size;
  unsigned long long addr;
} op_t;

static void print_cache_info(cache_t *cache);
static void parse_arg(cache_t *cache, char **trace_file, int argc, char *argv[]);
static void allocate_cache(cache_t *cache);
static void simulate(const char *trace_file, cache_t *cache);
static unsigned char *get_set(cache_t *cache, unsigned long long addr);
static unsigned char *get_line(cache_t *cache, unsigned char *set, int index);
static char is_valid(cache_t *cache, unsigned char *line);
static char match(cache_t *cache, unsigned char *line, unsigned long long addr);
static void cycle_shift_lines(cache_t *cache, unsigned char *set, int begin, int end);
static void update_line(cache_t *cache, unsigned char *line, unsigned long long addr);

static  int hits = 0;
static  int misses = 0;
static  int evictions = 0;

int main(int argc, char *argv[]) {
  cache_t cache;
  bzero(&cache, sizeof(cache));
  char *trace_file = NULL;


  parse_arg(&cache, &trace_file, argc, argv);
  allocate_cache(&cache);
  print_cache_info(&cache);
  simulate(trace_file, &cache);
  /* printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions); */
  printSummary(hits, misses, evictions);
  return 0;
}

void parse_arg(cache_t *cache, char **trace_file, int argc, char *argv[]) {
  char opt = 0;
  while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
    switch (opt) {
    case 's':
      cache->s = atoi(optarg);
      break;
    case 'E':
      cache->E = atoi(optarg);
      break;
    case 'b':
      cache->b = atoi(optarg);
      break;
    case 't':
      *trace_file = optarg;
      break;
    default:
      break;
    }
  }
  cache->t = 64 - cache->s - cache->b;
}

void allocate_cache(cache_t *cache) {
  /* cache->line_size = ((1 + cache->t + 7) / 8) + (1 << cache->b); */
  /* we are only counting hit/miss/eviction, no need to allocate for the payload */
  unsigned line_size = (1 + cache->t + 7) / 8;
  if (line_size <= 2) {

  } else if (2 < line_size && line_size <= 4) {
    line_size = 4;
  } else if (line_size <= 8) {
    line_size = 8;
  }
  cache->line_size = line_size;
  cache->set_size = cache->E * cache->line_size;
  cache->cache_size = (1 << cache->s) * cache->set_size;
  cache->data = malloc(cache->cache_size);
}

void print_cache_info(cache_t *cache) {
  printf("s\t\t%d\n", cache->s);
  printf("E\t\t%d\n", cache->E);
  printf("b\t\t%d\n", cache->b);
  printf("t\t\t%d\n", cache->t);
  printf("line size\t%d\n", cache->line_size);
  printf("set size\t%d\n", cache->set_size);
  printf("cache size\t%d\n", cache->cache_size);
}

#define BUFFSIZE 128

void simulate(const char *trace_file, cache_t *cache) {
  char *buff = malloc(BUFFSIZE);
  FILE *file = fopen(trace_file, "r");
  size_t len = 0;
  op_t op;

  while (getline(&buff, &len, file) != -1) {
    if (len == 0 || buff[0] == 'I') {
      continue;
    }

    if (sscanf(buff, " %c %llx,%d", &op.type, &op.addr, &op.size) != 3) {
      continue;
    }

    unsigned char *set = get_set(cache, op.addr);

    int line_idx = 0;
    for (line_idx = 0; line_idx < cache->E; ++line_idx) {
      unsigned char *line = get_line(cache, set, line_idx);
      if (is_valid(cache, line) && match(cache, line, op.addr)) {
        switch (op.type) {
        case 'L':
        case 'S':
          ++hits;
          break;
        case 'M':
          hits += 2;
          break;
        default:
          assert(0);
        }
        break;
      }
    }
    if (line_idx != cache->E) {
      /* matched */
      cycle_shift_lines(cache, set, 0, line_idx);
    } else {
      cycle_shift_lines(cache, set, 0, cache->E - 1);
      unsigned char *line = get_line(cache, set, 0);
      if (is_valid(cache, line)) {
        ++evictions;
      }
      update_line(cache, line, op.addr);
      switch (op.type) {
      case 'L':
      case 'S':
        ++misses;
        break;
      case 'M':
        ++misses;
        ++hits;
        break;
      default:
        assert(0);
        break;
      }
    }
  }

  fclose(file);
  free(buff);
}

unsigned char *get_set(cache_t *cache, unsigned long long addr) {
  int set_idx = (addr << cache->t) >> (cache->t + cache->b);
  return ((unsigned char *)(cache->data)) + (cache->set_size * set_idx);
}

unsigned char *get_line(cache_t *cache, unsigned char *set, int index) {
  assert(index < cache->E);
  return set + cache->line_size * index;
}

char is_valid(cache_t *cache, unsigned char *line) {
  /* valid bit the least significant bit */
  switch (cache->line_size) {
  case 1: {
    unsigned char data = *line;
    return 1 & data;
  }
  case 2: {
    unsigned short data = *(unsigned short*)line;
    return 1 & data;
  }
  case 4: {
    unsigned int data = *(unsigned int*)line;
    return 1 & data;
  }

  case 8: {
    unsigned int data = *(unsigned long long*)line;
    return 1 & data;
  }
  default:
    assert(0);
    break;
  }
  assert(0);
  return 0;
}

char match(cache_t *cache, unsigned char *line, unsigned long long addr) {
  unsigned long long tag = addr >> cache->b >> cache->s;
  switch (cache->line_size) {
  case 1: {
    unsigned char data = *line;
    return tag == (data >> 1);
  }
  case 2: {
    unsigned short data = *(unsigned short*)line;
    return tag == (data >> 1);
  }
  case 4: {
    unsigned int data = *(unsigned int*)line;
    return tag == (data >> 1);
  }
  case 8: {
    unsigned int data = *(unsigned long long*)line;
    return tag == (data >> 1);
  }
  default:
    assert(0);
    break;
  }
  assert(0);
  return 0;
}

void cycle_shift_lines(cache_t *cache, unsigned char *set, int begin, int end) {
  if (begin == end) {
    return;
  }
  char buff[8];
  unsigned char* last_line = get_line(cache, set, end);
  memcpy(buff, last_line, cache->line_size);

  for (int i = end; i > begin; --i) {
    unsigned char* this_line = get_line(cache, set, i);
    unsigned char* prev_line = get_line(cache, set, i - 1);
    memcpy(this_line, prev_line, cache->line_size);
  }
  unsigned char* first_line = get_line(cache, set, begin);
  memcpy(first_line, buff, cache->line_size);
}

void update_line(cache_t *cache, unsigned char *line, unsigned long long addr) {
  unsigned long long tag = addr >> (cache->b + cache->s);
  tag <<= 1;
  tag |= 1;
  switch (cache->line_size) {
  case 1:
    *(unsigned char*)line = (unsigned char)tag;
    break;
  case 2:
    *(unsigned short*)line = (unsigned short)tag;
    break;
  case 4:
    *(unsigned*)line = (unsigned)tag;
    break;
  case 8:
    *(unsigned long long*)line = (unsigned long long)tag;
    break;
  default:
    assert(0);
    break;
  }
}
