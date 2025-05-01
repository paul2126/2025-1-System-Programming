/*--------------------------------------------------------------------*/
/* This file is almost empty, and is provided to enable you to use    */
/* the Makefile.                                                      */
/* You are free to modify this file if you want.                      */
/* Even if you do not use this file, please keep it for the Makefile  */
/* and be sure to include it when you submit your assignment.         */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chunk.h"

struct Chunk {
  Chunk_T next; /* Pointer to the next chunk in the free chunk list */
  Chunk_T prev; /* Pointer to previous chunk in the free chunk list */
  int units;    /* Capacity of a chunk (chunk units) */
  int status;   /* CHUNK_FREE or CHUNK_IN_USE */
};

/*--------------------------------------------------------------------*/
int chunk_get_status(Chunk_T c) { return c->status; }
/*--------------------------------------------------------------------*/
void chunk_set_status(Chunk_T c, int status) { c->status = status; }
/*--------------------------------------------------------------------*/
int chunk_get_units(Chunk_T c) { return c->units; }
/*--------------------------------------------------------------------*/
void chunk_set_units(Chunk_T c, int units) { c->units = units; }
/*--------------------------------------------------------------------*/
void chunk_set_footer(Chunk_T c, int units) {
  // get footer address
  int *footer = (int *)((char *)c + (units + 1) * CHUNK_UNIT);
  *footer = units;
}
/*--------------------------------------------------------------------*/
Chunk_T chunk_get_next_free_chunk(Chunk_T c) { return c->next; }
/*--------------------------------------------------------------------*/
void chunk_set_next_free_chunk(Chunk_T c, Chunk_T next) {
  c->next = next;
}
/*--------------------------------------------------------------------*/
Chunk_T chunk_get_prev_free_chunk(Chunk_T c) { return c->prev; }
/*--------------------------------------------------------------------*/
void chunk_set_prev_free_chunk(Chunk_T c, Chunk_T prev) {
  c->prev = prev;
}
/*--------------------------------------------------------------------*/
Chunk_T chunk_get_next_adjacent(Chunk_T c, void *start, void *end) {
  // need to be refactored into double linked list
  Chunk_T n;

  assert((void *)c >= start);

  /* Note that a chunk consists of one chunk unit for a header footer
   * each, and many chunk units for data. */
  n = c + c->units + 2;

  /* If 'c' is the last chunk in memory space, then return NULL. */
  if ((void *)n >= end)
    return NULL;

  return n;
}

#ifndef NDEBUG
/*--------------------------------------------------------------------*/
int chunk_is_valid(Chunk_T c, void *start, void *end)
/* Return 1 (TRUE) iff c is valid */
{
  assert(c != NULL);
  assert(start != NULL);
  assert(end != NULL);

  if (c < (Chunk_T)start) {
    fprintf(stderr, "Bad heap start\n");
    return 0;
  }
  if (c >= (Chunk_T)end) {
    fprintf(stderr, "Bad heap end\n");
    return 0;
  }
  if (c->units == 0) {
    fprintf(stderr, "Zero units\n");
    return 0;
  }
  return 1;
}
#endif