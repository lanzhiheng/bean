#ifndef _BEAN_COMMON_H
#define _BEAN_COMMON_H
#include <stdlib.h>
#include <limits.h>
#include "bean.h"

#define bool char
#define byte char

#define bean_Number double

typedef unsigned char bu_byte;
typedef signed char bs_byte;

#define true 1
#define false 0
#define UNUSED __attribute__ ((unused))

typedef struct parser Parser;
typedef struct vm VM;

#define cast(t, exp)	((t)(exp))
#define cast_int(i)	cast(int, (i))
#define cast_char(i)	cast(char, (i))
#define cast_charp(i)	cast(char *, (i))
#define cast_uchar(i)	cast(unsigned char, (i))
#define cast_uint(i)	cast(unsigned int, (i))
#define cast_byte(i)	cast(bu_byte, (i))
#define cast_sizet(i)	cast(size_t, (i))

#define MAX_SIZET (~cast_sizet(0))
#define BEAN_IDSIZE	60

#if defined(DEBUG)
#include <assert.h>
#define bean_assert(c)           assert(c)
#else
#define bean_assert(c)           ((void)0)
#endif

#endif
