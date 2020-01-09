#ifndef BEAN_MEM_H
#define BEAN_MEM_H

#include "common.h"
#include "bstate.h"

typedef struct bean_State bean_State;

#define beanM_newobject(B,tag,s)	old_beanM_malloc_(B, (s), tag)

#define beanM_limitN(n, t) \
  ((cast_sizet(n) <= MAX_SIZET/sizeof(t)) ? (n) : cast_uint((MAX_SIZET/sizeof(t))))

#define beanM_reallocvchar(B, ptr, oldsize, newsize, t) \
  (cast(t *, beanM_realloc_(B, ptr, cast_sizet(oldsize) * sizeof(t),    \
                            cast_sizet(newsize) * sizeof(t))))

#define beanM_mallocvchar(B, size, t) (cast(t *, malloc(sizeof(t) * size)))

#define beanM_growvector(B, v, nelems, size, t, limit, e) \
  ((v) = cast(t *, beanM_grow_(B, v, nelems, &(size), sizeof(t), beanM_limitN(limit, t), e)))

#define beanM_malloc_(B, type) \
  (type *) beanM_realloc_(B, NULL, 0, sizeof(type))

#define beanM_array_malloc_(B, type, count)             \
  (type *) beanM_realloc_(B, NULL, 0, sizeof(type) * count)

#define beanM_array_dealloc_(B, arrayPtr, count)                \
  beanM_realloc_(B, NULL, sizeof(arrayPtr[0]) * count, 0)

void * beanM_realloc_(bean_State * B, void *ptr, size_t oldSize, size_t newSize);
void * beanM_grow_(bean_State * B, void * block, int n, int *psize, int size_elems, int limit, const char * what);
void * old_beanM_malloc_ (bean_State *B UNUSED, size_t size, int tag UNUSED);

#endif
