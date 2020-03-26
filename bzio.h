#ifndef BEAN_ZIO_H
#define BEAN_ZIO_H

#include "common.h"
#include "bstate.h"
#include "mem.h"

/* minimum size for string buffer */
#if !defined(BEAN_MINBUFFER)
#define BEAN_MINBUFFER	32
#endif

#define BUFFER_MAX INT_MAX

typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

#define beanZ_initbuffer(buff) ((buff)->buffer = beanM_mallocvchar(B, BEAN_MINBUFFER, char), (buff)->buffsize = BEAN_MINBUFFER, (buff)->n = 0)
#define beanZ_buffer(buff)	((buff)->buffer)
#define beanZ_sizebuffer(buff)	((buff)->buffsize)
#define beanZ_bufflen(buff)	((buff)->n)
#define beanZ_buffremove(buff,i)	((buff)->n -= (i))
#define beanZ_resetbuffer(buff) ((buff)->n = 0)

#define beanZ_resizebuffer(B, buff, size)                                \
  ((buff) -> buffer = beanM_realloc_(B, (buff) -> buffer, (buff)->buffsize, size), \
   (buff) -> buffsize = size)

#define beanZ_freebuffer(B, buff)	beanZ_resizebuffer(B, buff, 0)

// TODO 只有在虚拟机的时候会用到这个方法，扩容的时候尽量给予大的值避免内存溢出，目前原因未明。
#define beanZ_append(B, buff, c) do {                \
    if (buff->n >= buff->buffsize) {                 \
      beanZ_resizebuffer(B, buff, buff->buffsize * buff->buffsize); \
    }                                                \
    buff->buffer[buff->n++] = c;                     \
  } while(0);

#endif
