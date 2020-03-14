#include "mem.h"
#include "berror.h"

/*
** Minimum size for arrays during parsing, to avoid overhead of
** reallocating to size 1, then 2, and then 4. All these arrays
** will be reallocated to exact sizes or erased when parsing ends.
*/
#define MINSIZEARRAY	4

void * beanM_realloc_(bean_State * B, void *ptr, size_t oldSize, size_t newSize) {
  B -> allocateBytes += newSize - oldSize;

  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  void * p = realloc(ptr, newSize);
  if (p == NULL) {
    mem_error(B, "memory allocated issue\n");
  }
  return p;
}

void * beanM_grow_(bean_State * B, void * block, int n, int *psize, int size_elems, int limit, const char * what) {
  void * newblock;
  int size = *psize;

  if (n + 1 <= size) {
    return block;
  }

  if (size >= limit / 2) {
    if (size > limit) mem_error(B, "too many %s (limit is %d)", what, limit);
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
