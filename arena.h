#pragma once
/*
this is a arena allocator

example
===
#define CREATE_ARENA
#include "arena.h"

int main() {
  atexit(&Arena_free_all);
  char *str = (char *)Arena_alloc(26);
  for (int i = 0; i < 26; i++) {
    str[i] = 'a' + i;
  }
  printf("%s\n", str);
  Arena_free(str);
  return 0;
}

cc main.c -o main -O4 -Oz -Wall -Wextra -Wpedantic -Werror
strip main
./main
abcdefghijklmnopqrstuvwxyz
[arena]freeing all
===

is this thread safe? Fuck no, why would you want to allocate on the heap in
multiple threads?
Same thing for async

if you think any of the declarations/implementations are wrong, you seriusly
need to read the fucking gnu headers, like stdlib.h

// Shorthand for type of comparison functions.  //
#ifndef __COMPAR_FN_T
# define __COMPAR_FN_T
typedef int (*__compar_fn_t) (const void *, const void *);

# ifdef __USE_GNU
typedef __compar_fn_t comparison_fn_t;
# endif
#endif
#ifdef __USE_GNU
typedef int (*__compar_d_fn_t) (const void *, const void *, void *);
#endif

// Do a binary search for KEY in BASE, which consists of NMEMB elements
// of SIZE bytes each, using COMPAR to perform the comparisons.
extern void *bsearch (const void *__key, const void *__base,
          size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __nonnull ((1, 2, 5)) __wur;


*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> // for once gnu/whoever maintains the headers actualy used their brain
                       // typedef unsigned int uint;

#if !defined(HEAP_USE_LESS_RAM) && !defined(HEAP_USE_CUSTOM_RAM)
// const CHUNK_LIST_CAP int = 1024
#define CHUNK_LIST_CAP 1024
// 64 mb by default
#define HEAP_CAP (64 * 1024 * 1024)
#elif defined(HEAP_USE_CUSTOM_RAM)
#define HEAP_CAP (HEAP_USE_CUSTOM_RAM_MB * 1024 * 1024)
#define CHUNK_LIST_CAP HEAP_USE_CUSTOM_CAP
#else
// ~3MB of ram for heap
// const CHUNK_LIST_CAP int = 64
#define CHUNK_LIST_CAP 64
// 16 mb if you want less memory used by the arena
#define HEAP_CAP (16 * 1024 * 1024)
#endif

void *Arena_alloc(uint size);
void Arena_free(void *p);
void Arena_collect();
void Arena_free_all();
void Arena_assert(bool b, const char *msg);
// this as well
// void *bsearch(void *key, void *base, uint count, uint size, void *cmpFn);

#ifdef CREATE_ARENA
void Arena_assert(bool b, const char *msg) {
  if (!b) {
    printf("%s\n", msg);
    abort();
  }
}
typedef struct Chunk {
  char *start;
  uint size;
} Chunk;

typedef struct ChunkList {
  uint count;
  Chunk chunks[CHUNK_LIST_CAP];
} ChunkList;
static char heap[HEAP_CAP] = {0};
uint heapSize = 0;
static ChunkList alloced_chunks = {0};
static ChunkList freed_chunks = {0};
// typedef int (*__compar_fn_t) (const void *, const void *);
int chunk_start_cmp(const void *a, const void *b) {
  Chunk *ac = (Chunk *)a;
  Chunk *bc = (Chunk *)b;
  return (ac->start - bc->start);
}

/*
extern void *bsearch (const void *__key, const void *__base,
          size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __nonnull ((1, 2, 5)) __wur;
*/
int chunk_list_find(ChunkList *list, void *p) {
  Chunk key = (Chunk){(char *)p, 0};
  Chunk *thing = (Chunk *)bsearch(&key, list->chunks, list->count,
                                  sizeof(list->chunks[0]), &chunk_start_cmp);
  if (thing) {
    Arena_assert(list->chunks <= (Chunk *)thing,
                 "returned pointer is out of bounds of the ChunkList");
    return (int)(thing - list->chunks);
  } else {
    return -1;
  }
}

#ifdef ARENA_DEBUG
void chunk_list_dump(ChunkList *list) {
  int size = (int)list->count;
  printf("chunks (%i)\n", size);
  for (int i = 0; i < size; i++) {
    printf("start: %p, size: %i\n", list->chunks[(int)i].start,
           list->chunks[(int)i].size);
  }
}
#endif

void chunk_list_insert(ChunkList *list, void *start, uint size) {
  Arena_assert(list->count < CHUNK_LIST_CAP,
               "Cannot insert any more items into heap");
  list->chunks[list->count].start = (char *)start;
  list->chunks[list->count].size = size;
  int __start = (int)list->count - 1;
  for (int i = __start; i > 0; i--) {
    if (list->chunks[i].start >= list->chunks[i - 1].start) {
      break;
    }
    Chunk temp = list->chunks[i];
    list->chunks[i] = list->chunks[i - 1];
    list->chunks[i - 1] = temp;
  }
  list->count++;
}

void chunk_list_remove(ChunkList *list, int index) {
  Arena_assert(index < (int)list->count, "invalid index");
  int s = (int)list->count - 1;
  for (int i = index; i < s; i++) {
    list->chunks[i] = list->chunks[i + 1];
  }
  list->count--;
}

void *Arena_alloc(uint size) {
  size = (size + 7) & ~7;

  uint fr = freed_chunks.count;
  for (int i = 0; i < (int)fr; i++) {
    Chunk chunk = freed_chunks.chunks[i];
    chunk_list_insert(&alloced_chunks, chunk.start, chunk.size);
    if (chunk.size > size) {
      void *leftover_start = chunk.start + size;
      uint leftover_size = chunk.size - size;
      chunk_list_insert(&freed_chunks, leftover_start, leftover_size);
    }
    chunk_list_remove(&freed_chunks, i);
    i--;
    return chunk.start;
  }
  Arena_assert(heapSize + size <= HEAP_CAP, "Failed to allocate in heap");
  void *res = heap + heapSize;
  heapSize += size;
  chunk_list_insert(&alloced_chunks, res, size);
  return res;
}

void Arena_free(void *area) {
  if (!area) {
    printf("cannot free some pointer that doesn't belong in the arena");
    abort();
  }

  int index = chunk_list_find(&alloced_chunks, area);
  Arena_assert(index >= 0, "does this block of memory even exist in the heap?");
  chunk_list_insert(&freed_chunks, alloced_chunks.chunks[index].start,
                    alloced_chunks.chunks[index].size);
  chunk_list_remove(&alloced_chunks, index);
}
void Arena_collect() {
  uint i = 0;
  uint chunks = freed_chunks.count;
  while (i + 1 < chunks) {
    Chunk current = freed_chunks.chunks[i];
    Chunk next = freed_chunks.chunks[i + 1];

    char *current_end = current.start + current.size;

    if (current_end == next.start) {
      // merging
      freed_chunks.chunks[i].size += next.size;
      chunk_list_remove(&freed_chunks, i + 1);
      // don't increment, re-check new next
    } else {
      i += 1;
    }
  }
  if (freed_chunks.count == 1) {
    Chunk only = freed_chunks.chunks[0];

    if ((void *)only.start == heap && only.size == heapSize) {
      heapSize = 0;
      freed_chunks.count = 0;
    }
  }
}
void Arena_free_all() {
  // this likely needs to do something else, like going over every chunk and
  // freeing it, but for now this will do
  printf("[arena]freeing all\n");
  heapSize = 0;
  freed_chunks.count = 0;
  alloced_chunks.count = 0;
}
#endif
