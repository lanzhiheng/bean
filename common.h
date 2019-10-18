#ifndef _BEAN_COMMON_H
#define _BEAN_COMMON_H
#include <stdlib.h>

#define bool char
#define byte char

typedef unsigned char lu_byte;
typedef signed char ls_byte;

#define true 1
#define false 0
#define UNUSED __attribute__ ((unused))

typedef struct parser Parser;
typedef struct vm VM;

#define cast(t, exp)	((t)(exp))
#define cast_int(i)	cast(int, (i))
#define cast_char(i)	cast(char, (i))

/* minimum size for string buffer */
#if !defined(BEAN_MINBUFFER)
#define BEAN_MINBUFFER	32
#endif

#define bean_Integer long
#define bean_Number long double

#if defined(DEBUG)
#include <assert.h>
#define ASSERT(c)           assert(c)
#else
#define ASSERT(c)           ((void)0)
#endif

#endif
