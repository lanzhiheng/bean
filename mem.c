#include "mem.h"

void * beanM_malloc_ (bean_State *B UNUSED, size_t size, int tag UNUSED) {
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
