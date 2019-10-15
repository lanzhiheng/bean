#ifndef BEAN_ZIO_H
#define BEAN_ZIO_H

#include "common.h"
#include "bstate.h"

typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

void * beanM_reallocvchar(bean_State * B, void *ptr, size_t oldSize, size_t newSize);

#define beanZ_initbuffer(buff) ((buff)->buffer = NULL, (buff)->buffsize = 0)
#define beanZ_buffer(buff)	((buff)->buffer)
#define beanZ_sizebuffer(buff)	((buff)->buffsize)
#define beanZ_bufflen(buff)	((buff)->n)

#define beanZ_buffremove(buff,i)	((buff)->n -= (i))
#define beanZ_resetbuffer(buff) ((buff)->n = 0)

#define beanZ_resizebuffer(B, buff, size)                                \
  ((buff) -> buffer = beanM_reallocvchar(B, (buff) -> buffer, (buff)->buffsize, size), \
   (buff) -> buffsize = size)

#define beanZ_freebuffer(B, buff)	luaZ_resizebuffer(B, buff, 0)

#endif
