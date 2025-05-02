#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

#define FALSE 0
#define TRUE 1

enum {
  MEMALLOC_MIN = 1024,
};
// static void dump_heap_to_dot(const char *filename);

int iteration = 0;
/* g_free_head: point to first chunk in the free list */
static Chunk_T g_free_head = NULL;

/* g_heap_start, g_heap_end: start and end of the heap area.
 * g_heap_end will move if you increase the heap */
static void *g_heap_start = NULL, *g_heap_end = NULL;

#ifndef NDEBUG
/* check_heap_validity:
 * Validity check for entire data structures for chunks. Note that this
 * is basic sanity check, and passing this test does not guarantee the
 * integrity of your code.
 * Returns 1 on success or 0 (zero) on failure.
 */
static int check_heap_validity(void) {
  Chunk_T w;

  if (g_heap_start == NULL) {
    fprintf(stderr, "Uninitialized heap start\n");
    return FALSE;
  }

  if (g_heap_end == NULL) {
    fprintf(stderr, "Uninitialized heap end\n");
    return FALSE;
  }

  if (g_heap_start == g_heap_end) {
    if (g_free_head == NULL)
      return 1;
    fprintf(stderr, "Inconsistent empty heap\n");
    return FALSE;
  }

  for (w = (Chunk_T)g_heap_start; w && w < (Chunk_T)g_heap_end;
       w = chunk_get_next_adjacent(w, g_heap_start, g_heap_end)) {

    if (!chunk_is_valid(w, g_heap_start, g_heap_end))
      return 0;
  }

  for (w = g_free_head; w; w = chunk_get_next_free_chunk(w)) {
    Chunk_T n;

    if (chunk_get_status(w) != CHUNK_FREE) {
      fprintf(stderr, "Non-free chunk in the free chunk list\n");
      return 0;
    }

    if (!chunk_is_valid(w, g_heap_start, g_heap_end))
      return 0;

    n = chunk_get_next_adjacent(w, g_heap_start, g_heap_end);
    if (n != NULL && n == chunk_get_next_free_chunk(w)) {
      fprintf(stderr, "Uncoalesced chunks\n");
      return 0;
    }
  }
  return TRUE;
}
#endif

/*--------------------------------------------------------------*/
/* size_to_units:
 * Returns capable number of units for 'size' bytes.
 */
/*--------------------------------------------------------------*/
static size_t size_to_units(size_t size) { // ok
  return (size + (CHUNK_UNIT - 1)) / CHUNK_UNIT;
}
/*--------------------------------------------------------------*/
/* get_chunk_from_data_ptr:
 * Returns the header pointer that contains data 'm'.
 */
/*--------------------------------------------------------------*/
static Chunk_T get_chunk_from_data_ptr(void *m) {
  return (Chunk_T)((char *)m - CHUNK_UNIT);
}
/*--------------------------------------------------------------------*/
/* init_my_heap:
 * Initialize data structures and global variables for
 * chunk management.
 */
/*--------------------------------------------------------------------*/
static void init_my_heap(void) {
  /* Initialize g_heap_start and g_heap_end */
  g_heap_start = g_heap_end = sbrk(0);
  if (g_heap_start == (void *)-1) {
    fprintf(stderr, "sbrk(0) failed\n");
    exit(-1);
  }
}
/*--------------------------------------------------------------------*/
/* merge_chunk:
 * Merge two adjacent chunks and return the merged chunk. must be always
 * c1 c2 order Returns NULL on error.
 */
/*--------------------------------------------------------------------*/
static Chunk_T merge_chunk(Chunk_T c1, Chunk_T c2) {
  /* c1 and c2 must be be adjacent */
  assert(c1 < c2 &&
         chunk_get_next_adjacent(c1, g_heap_start, g_heap_end) == c2);
  assert(chunk_get_status(c1) == CHUNK_FREE);
  assert(chunk_get_status(c2) == CHUNK_FREE);

  /* adjust the units and the next pointer of c1 */
  chunk_set_units(c1, chunk_get_units(c1) + chunk_get_units(c2) + 2);
  chunk_set_next_free_chunk(c1, chunk_get_next_free_chunk(c2));

  // set prev free chunk of n(free chunk after c2) to c1
  Chunk_T n = chunk_get_next_free_chunk(c2);
  if (n != NULL) // check for end of list
    chunk_set_prev_free_chunk(n, c1);
  return c1;
}
/*--------------------------------------------------------------------*/
/* split_chunk:
 * Split 'c' into two chunks s.t. the size of one chunk is 'units' and
 * the size of the other chunk is (original size - 'units' - 1).
 * returns the chunk with 'units'
 * Returns the data chunk. */
/*--------------------------------------------------------------------*/
static Chunk_T split_chunk(Chunk_T c, size_t units) {
  Chunk_T c2;
  size_t all_units;

  assert(c >= (Chunk_T)g_heap_start && c <= (Chunk_T)g_heap_end);
  assert(chunk_get_status(c) == CHUNK_FREE);
  assert(chunk_get_units(c) >
         units + 2); /* assume chunk with header&footer unit */

  /* adjust the size of the first chunk (free) */
  all_units = chunk_get_units(c);
  chunk_set_units(c, all_units - units - 2); // header & footer
  chunk_set_footer(c, all_units - units - 2);

  /* prepare for the second chunk (use)*/
  c2 = chunk_get_next_adjacent(c, g_heap_start, g_heap_end);
  chunk_set_units(c2, units);
  chunk_set_status(c2, CHUNK_IN_USE);

  // set footer
  chunk_set_footer(c2, units);
  chunk_set_next_free_chunk(c2, chunk_get_next_free_chunk(c));

  return c2;
}
/*--------------------------------------------------------------------*/
/* insert_chunk:
 * Insert a chunk, 'c', into the head of the free chunk list.
 * 'c' will be merged with the first chunk if possible.
 * The status of 'c' is set to CHUNK_FREE
 */
/*--------------------------------------------------------------------*/
static void insert_chunk(Chunk_T c) {
  assert(chunk_get_units(c) >= 1);

  chunk_set_status(c, CHUNK_FREE);

  /* If the free chunk list is empty, chunk c points to itself. */
  if (g_free_head == NULL) {
    g_free_head = c;
    chunk_set_next_free_chunk(c, NULL);
  } else {
    assert(c < g_free_head);
    chunk_set_next_free_chunk(c, g_free_head);
    if (chunk_get_next_adjacent(c, g_heap_start, g_heap_end) ==
        g_free_head)
      merge_chunk(c, g_free_head);
    g_free_head = c;
  }
}
/*--------------------------------------------------------------------*/
/* insert_chunk_after:
 * Insert a chunk, 'c', to the free chunk list after 'e' which is
 * already in the free chunk list. After the operation, 'c' will be the
 * next element of 'e' in the free chunk list.
 * 'c' will be merged with neighbor free chunks if possible.
 * It will return 'c' (after the merge operation if possible).
 */
/*--------------------------------------------------------------------*/
static Chunk_T insert_chunk_after(Chunk_T e, Chunk_T c) {
  Chunk_T n;

  assert(e < c);
  assert(chunk_get_status(e) == CHUNK_FREE);
  assert(chunk_get_status(c) != CHUNK_FREE);

  // set prev for c+1 chunk to c
  n = chunk_get_next_free_chunk(e);
  if (n != NULL)
    chunk_set_prev_free_chunk(n, c);

  chunk_set_next_free_chunk(c, chunk_get_next_free_chunk(e));
  chunk_set_next_free_chunk(e, c);
  chunk_set_prev_free_chunk(c, e);

  chunk_set_status(c, CHUNK_FREE);

  /* see if c can be merged with e */
  if (chunk_get_next_adjacent(e, g_heap_start, g_heap_end) == c)
    c = merge_chunk(e, c);

  /* see if we can merge with n */
  n = chunk_get_next_adjacent(c, g_heap_start, g_heap_end);
  if (n != NULL && chunk_get_status(n) == CHUNK_FREE)
    c = merge_chunk(c, n);

  return c;
}
/*--------------------------------------------------------------------*/
/* remove_chunk_from_list:
 * Removes 'c' from the free chunk list. 'prev' should be the previous
 * free chunk of 'c' or NULL if 'c' is the first chunk
 */
/*--------------------------------------------------------------------*/
static void remove_chunk_from_list(Chunk_T prev, Chunk_T c) {
  assert(chunk_get_status(c) == CHUNK_FREE);

  Chunk_T n;

  if (prev == NULL)
    g_free_head = chunk_get_next_free_chunk(c);
  else
    chunk_set_next_free_chunk(prev, chunk_get_next_free_chunk(c));

  // get next chunk and set prev chunk
  n = chunk_get_next_free_chunk(c);
  if (n != NULL)
    chunk_set_prev_free_chunk(n, prev);

  chunk_set_next_free_chunk(c, NULL);
  chunk_set_status(c, CHUNK_IN_USE);
}
/*--------------------------------------------------------------------*/
/* allocate_more_memory:
 * Allocate a new chunk which is capable of holding 'units' chunk
 * units in memory by increasing the heap, and return the new
 * chunk. 'prev' should be the last chunk in the free list.
 * This function also performs chunk merging with "prev" if possible
 * after allocating a new chunk.
 */
/*--------------------------------------------------------------------*/
static Chunk_T allocate_more_memory(Chunk_T prev, size_t units) {
  Chunk_T c;

  if (units < MEMALLOC_MIN)
    units = MEMALLOC_MIN;

  /* Note that we need to allocate two more unit for header & footer. */
  c = (Chunk_T)sbrk((units + 2) * CHUNK_UNIT);
  if (c == (Chunk_T)-1)
    return NULL;

  g_heap_end = sbrk(0);
  chunk_set_units(c, units);
  chunk_set_next_free_chunk(c, NULL);
  chunk_set_status(c, CHUNK_IN_USE);

  chunk_set_footer(c, units);         // set footer
  chunk_set_prev_free_chunk(c, prev); // set prev chunk for c

  /* Insert the newly allocated chunk 'c' to the free list.
   * Note that the list is sorted in ascending order. */
  if (g_free_head == NULL)
    insert_chunk(c);
  else
    c = insert_chunk_after(prev, c);

  assert(check_heap_validity());
  return c;
}
/*--------------------------------------------------------------*/
/* heapmgr_malloc:
 * Dynamically allocate a memory capable of holding size bytes.
 * Substitute for GNU malloc().
 */
/*--------------------------------------------------------------*/
void *heapmgr_malloc(size_t size) {
  static int is_init = FALSE;
  Chunk_T c, prev, pprev;
  size_t units;

  if (size <= 0)
    return NULL;

  if (is_init == FALSE) {
    init_my_heap();
    is_init = TRUE;
  }

  /* see if everything is OK before doing any operations */
  assert(check_heap_validity());

  units = size_to_units(size);

  pprev = NULL;
  prev = NULL;

  // for (c = g_free_head; c != NULL; c = chunk_get_next_free_chunk(c))
  // {

  //   if (chunk_get_units(c) >= units) {
  //     // remaining chunk should be big enough to have header and
  //     footer if (chunk_get_units(c) >= units + 2)
  //       c = split_chunk(c, units);
  //     else if (chunk_get_units(c) == units) // perfect fit
  //       remove_chunk_from_list(prev, c);    // use this chunk. remove
  //       it
  //     else { // not big enough for header & footer
  //       pprev = prev;
  //       prev = c;
  //       continue; // not enough space for header and footer
  //     }

  //     assert(check_heap_validity());
  //     return (void *)((char *)c + CHUNK_UNIT);
  //   }
  //   pprev = prev;
  //   prev = c;
  // }
  for (c = g_free_head; c != NULL; c = chunk_get_next_free_chunk(c)) {

    if (chunk_get_units(c) >= units) {
      if (chunk_get_units(c) > units + 2)
        c = split_chunk(c, units);
      else
        remove_chunk_from_list(prev, c);

      assert(check_heap_validity());
      return (void *)((char *)c + CHUNK_UNIT);
    }
    pprev = prev;
    prev = c;
  }

  /* allocate new memory */
  c = allocate_more_memory(prev, units);
  if (c == NULL) {
    assert(check_heap_validity());
    return NULL;
  }
  assert(chunk_get_units(c) >= units);

  /* if c was merged with prev, pprev becomes prev */
  if (c == prev)
    prev = pprev;

  if (chunk_get_units(c) >= units + 2) // header & footer
    c = split_chunk(c, units);
  else
    remove_chunk_from_list(prev, c);

  assert(check_heap_validity());

  // print heap to dot file
  // dump_heap_to_dot("malloc-heap.dot");
  return (void *)((char *)c + CHUNK_UNIT);
}
/*--------------------------------------------------------------*/
/* heapmgr_free:
 * Releases dynamically allocated memory.
 * Substitute for GNU free().                                   */
/*--------------------------------------------------------------*/
void heapmgr_free(void *m) {
  Chunk_T c, w, prev;

  if (m == NULL)
    return;

  /* check everything is OK before freeing 'm' */
  assert(check_heap_validity());

  /* get the chunk header pointer from m */
  c = get_chunk_from_data_ptr(m);
  assert(chunk_get_status(c) != CHUNK_FREE);

  prev = NULL;

  /* traverse the free chunk list to find the
     right location for 'c' */
  for (w = g_free_head; w != NULL; w = chunk_get_next_free_chunk(w)) {
    if (c < w)
      break;
    prev = w;
  }

  /* insert the new free chunk into the free list */
  if (prev == NULL)
    insert_chunk(c);
  else
    insert_chunk_after(prev, c);

  /* double check if everything is OK */
  assert(check_heap_validity());

  // print heap to dot file
  // dump_heap_to_dot("free-heap.dot");
}
/*
static void dump_heap_to_dot(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    perror("fopen");
    return;
  }

  fprintf(f, "digraph CombinedHeap {\n");
  fprintf(f, "  node [shape=record];\n");

  // dump entire heap
  Chunk_T w;
  for (w = (Chunk_T)g_heap_start; (void *)w < g_heap_end;
       w = chunk_get_next_adjacent(w, g_heap_start, g_heap_end)) {
    fprintf(f,
            "  chunk_%p [label=\"{Address: %p | Size: %d | Status: "
            "%s}\"];\n",
            (void *)w, (void *)w, chunk_get_units(w),
            chunk_get_status(w) == CHUNK_FREE ? "FREE" : "IN_USE");

    // link to next adjacent chunk
    Chunk_T next = chunk_get_next_adjacent(w, g_heap_start,
    g_heap_end); if (next != NULL) {
      fprintf(f, "  chunk_%p -> chunk_%p [label=\"adjacent\"];\n",
              (void *)w, (void *)next);
    } else {
      break;
    }
  }

  // dump free list
  for (w = g_free_head; w != NULL; w = chunk_get_next_free_chunk(w))
  {
    // next (next_free_chunk)
    if (chunk_get_next_free_chunk(w)) {
      fprintf(f,
              "  chunk_%p -> chunk_%p [label=\"next\""
              "color =\"green\"];\n",
              (void *)w, (void *)chunk_get_next_free_chunk(w));
    }

    // prev (prev_free_chunk)
    if (chunk_get_prev_free_chunk(w)) {
      fprintf(f,
              "  chunk_%p -> chunk_%p [label=\"prev\""
              "color =\"blue\"];\n",
              (void *)w, (void *)chunk_get_prev_free_chunk(w));
    }
  }

  fprintf(f, "}\n");
  fclose(f);
}
*/