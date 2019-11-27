#include "mem.h"
#include "bdebug.h"

/*
** Minimum size for arrays during parsing, to avoid overhead of
** reallocating to size 1, then 2, and then 4. All these arrays
** will be reallocated to exact sizes or erased when parsing ends.
*/
#define MINSIZEARRAY	4

// TODO: Remove it later
void * old_beanM_malloc_ (bean_State *B UNUSED, size_t size, int tag UNUSED) {
  void * ptr = malloc(size);
  return ptr;
}

void * beanM_realloc_(bean_State * B, void *ptr, size_t oldSize, size_t newSize) {
  B -> allocateBytes += newSize - oldSize;

  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, newSize);
}

void * beanM_grow_(bean_State * B, void * block, int n, int *psize, int size_elems, int limit, const char * what) {
  void * newblock;
  int size = *psize;

  if (n + 1 <= size) {
    return block;
  }

  if (size >= limit / 2) {
    if (size > limit) beanG_runerror(B, "too many %s (limit is %d)", what, limit);
    size = limit;  /* still have at least one free place */
  } else {
    size *= 2;

    if (size < MINSIZEARRAY)
      size = MINSIZEARRAY;  /* minimum size */
  }

  newblock = beanM_realloc_(B, block, cast_sizet(*psize) * size_elems,
                                         cast_sizet(size) * size_elems);

  *psize = size;
  return newblock;
}
