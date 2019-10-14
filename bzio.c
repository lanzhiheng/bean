#include "bzio.h"

void * beanM_reallocvchar(bean_State * B, void *ptr, size_t oldSize, size_t newSize) {
  B -> allocateBytes += newSize - oldSize;

  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, newSize);
}
