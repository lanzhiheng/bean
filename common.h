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
typedef struct value Value;


#define cast(t, exp)	((t)(exp))
#define cast_int(i)	cast(int, (i))

/* minimum size for string buffer */
#if !defined(BEAN_MINBUFFER)
#define BEAN_MINBUFFER	32
#endif

#endif
