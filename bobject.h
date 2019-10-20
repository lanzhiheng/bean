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
  byte tt_;
} TValue;

/*
** Some helper macro
**
*/

#define check_exp(c,e)		(ASSERT(c), (e))
/* raw type tag of a TValue */
#define rawtt(o)	((o)->tt_)

/* tag with no variants (bits 0-3) */
#define novariant(t)	((t) & 0x0F)

/* type of a TValue */
#define ttype(o)	(novariant(rawtt(o)))

/* Macros to test type */
#define checktag(o,t)		(rawtt(o) == (t))
#define checktype(o,t)		(ttype(o) == (t))

/* Macros to set values */
#define settt_(o,t)	((o)->tt_=(t))

/* Macros to get value_ of TValue */
#define val_(o)		((o)->value_)

/* }================================================================== */

/*
** {==================================================================
** Numbers
** ===================================================================
*/

#define BEAN_TNUMFLT	(BEAN_TNUMBER | (1 << 4))  /* float numbers */
#define BEAN_TNUMINT	(BEAN_TNUMBER | (2 << 4))  /* integer numbers */

#define ttisnumber(o)		checktype((o), BEAN_TNUMBER)
#define ttisfloat(o)		checktag((o), BEAN_TNUMFLT)
#define ttisinteger(o)		checktag((o), BEAN_TNUMINT)

#define nvalue(o)       check_exp(ttisnumber(o),                        \
                                  (ttisinteger(o) ? cast_num(ivalue(o)): fltvalue(o)))
#define ivalue(o)       check_exp(ttisinteger(o), val_(o).i)
#define fltvalue(o)       check_exp(ttisfloat(o), val_(o).n)

#define setivalue(obj,x)                                                \
  { TValue *io = obj; val_(io).i=(x); settt_(io, BEAN_TNUMINT); }

#define setfltvalue(obj,x) \
  { TValue *io = obj; val_(io).n=(x); settt_(io, BEAN_TNUMFLT); }

/* }================================================================== */

#endif
