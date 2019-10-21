#include "mem.h"

void *beanM_malloc_ (bean_State *B, size_t size, int tag UNUSED) {
  void * ptr = beanM_reallocvchar(B, NULL, 0, size);

  return ptr;
}

void * beanM_reallocvchar(bean_State * B, void *ptr, size_t oldSize, size_t newSize) {
  B -> allocateBytes += newSize - oldSize;

  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, newSize);
}
