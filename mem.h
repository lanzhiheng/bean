#ifndef BEAN_MEM_H
#define BEAN_MEM_H

#include "common.h"
#include "bstate.h"

#define beanM_newobject(B,tag,s)	beanM_malloc_(B, (s), tag)

void * beanM_reallocvchar(bean_State * B, void *ptr, size_t oldSize, size_t newSize);

void *beanM_malloc_ (bean_State *B, size_t size, int tag);

#endif
