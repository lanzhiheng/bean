#ifndef _BEAN_OBJECT_H
#define _BEAN_OBJECT_H

#include <stdlib.h>
#include "common.h"

#define MAX_STRING_BUFFER 256

/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	struct GCObject *next; bu_byte tt; bu_byte marked

typedef struct bean_State bean_State;

/* Common type for all collectable objects */
typedef struct GCObject {
  CommonHeader;
} GCObject;

typedef struct TString {
  CommonHeader;
  size_t len;  /* length of string */
  unsigned int hash;
  bu_byte reserved;
  struct TString *hnext;  /* linked list for hash table */
} TString;

typedef union Value {
  struct GCObject * gc;
  bean_Integer i;
  bean_Number n;
  TString * s;
  struct Function * fc;
  struct Tool * tl;
  bu_byte b;
} Value;

#define TValuefields   Value value_; bu_byte tt_

typedef struct TValue {
  TValuefields;
} TValue;

/*
** Some helper macro
**
*/

#define fvalue(o) (ttisfalse(o) || ttisnil(o))
#define tvalue(o) !fvalue(o)


/*
** 'module' operation for hashing (size is always a power of 2)
*/
#define bmod(s,size) \
        (check_exp((size&(size-1))==0, (cast_int((s) & ((size)-1)))))

#define check_exp(c,e)		(bean_assert(c), (e))
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

#define ttisnil(o)           checktype((o), BEAN_TNIL)
#define ttisboolean(o)		checktype((o), BEAN_TBOOLEAN)
#define ttisfalse(o)         (ttisboolean(o) && !val_(o).b)
#define ttistrue(o)         (ttisboolean(o) && val_(o).b)

#define setnilvalue(obj)                                                \
  { TValue *io = obj; settt_(io, BEAN_TNIL); }
#define setbvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).b=(x); settt_(io, BEAN_TBOOLEAN); }
#define bvalue(o)       check_exp(ttisboolean(o), val_(o).b)

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

#define ttisstring(o)		checktag((o), BEAN_TSTRING)
#define ttisfunction(o)		checktag((o), BEAN_TFUNCTION)
#define ttistool(o)		checktag((o), BEAN_TTOOL)
#define svalue(o)       check_exp(ttisstring(o), val_(o).s)

#define setsvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).s=(x); settt_(io, BEAN_TSTRING); }

#define setfcvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).fc=(x); settt_(io, BEAN_TFUNCTION); }

#define settlvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).tl=(x); settt_(io, BEAN_TTOOL); }

#define fcvalue(o)       check_exp(ttisfunction(o), val_(o).fc)
#define tlvalue(o)       check_exp(ttistool(o), val_(o).tl)

bool tvalue_equal(TValue * v1, TValue * v2);
TValue * tvalue_inspect(bean_State * B, TValue * value);
#endif
