#ifndef _BEAN_COMMON_H
#define _BEAN_COMMON_H
#include "stdint.h"

#define bool char
#define byte char
#define true 1
#define false 0
#define UNUSED __attribute__ ((unused))

typedef struct parser Parser;
typedef struct vm VM;
typedef struct value Value;
#endif
