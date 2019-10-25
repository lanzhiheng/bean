#ifndef BEAN_MEM_H
#define BEAN_MEM_H

#include "common.h"
#include "bstate.h"

#define beanM_newobject(B,tag,s)	beanM_malloc_(B, (s), tag)

#define beanM_limitN(n, t) \
  ((cast_sizet(n) <= MAX_SIZET/sizeof(t)) ? (n) : cast_uint((MAX_SIZET/sizeof(t))))

#define MEM_ERROR(B, message) printf(message)

#define beanM_reallocvchar(B, ptr, oldsize, newsize, t) \
  (cast(t *, beanM_realloc_(B, ptr, cast_sizet(oldsize) * sizeof(t),    \
                            cast_sizet(newsize) * sizeof(t))))

#define beanM_mallocvchar(B, size, t) (cast(t *, malloc(sizeof(t) * size)))

void * beanM_malloc_ (bean_State *B, size_t size, int tag);
void * beanM_realloc_(bean_State * B, void *ptr, size_t oldSize, size_t newSize);

#endif
