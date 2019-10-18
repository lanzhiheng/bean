#ifndef _BEAN_OBJECT_H
#define _BEAN_OBJECT_H

#include "common.h"
#include <stdlib.h>

/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	struct GCObject *next; lu_byte tt; lu_byte marked

/* Common type for all collectable objects */
typedef struct GCObject {
  CommonHeader;
} GCObject;

typedef struct TString {
  CommonHeader;
  size_t lnglen;  /* length for long strings */
  unsigned int hash;
} TString;

typedef union Value {
  struct GCObject * gc;
  bean_Integer i;
  bean_Number n;
} Value;

typedef struct TValue {
  Value value_;
  byte tt;
} TValue;

#endif
