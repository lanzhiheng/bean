#ifndef _BEAN_OBJECT_H
#define _BEAN_OBJECT_H

#include <stdlib.h>
#include <regex.h>
#include "common.h"


#define MAX_STRING_BUFFER 256

/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	struct GCObject *next; bu_byte tt; bu_byte marked

typedef struct bean_State bean_State;
typedef struct expr expr;

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

typedef struct Regex {
  CommonHeader;
  regex_t rr;
  char * match;
  int mode;
} Regex;

typedef union Value {
  struct GCObject * gc;
  bean_Number n;
  TString * s;
  Regex * rr;
  struct Function * fc;
  struct Tool * tl;
  struct Array * ar;
  struct Hash * hh;
  bu_byte b;
} Value;

#define TValuefields   Value value_; bu_byte tt_

typedef struct TValue {
  TValuefields;
  struct TValue * prototype;
} TValue;

/*
** Some helper macro
**
*/

#define falsyvalue(o) (ttisfalse(o) || ttisnil(o))
#define truthvalue(o) !falsyvalue(o)


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
  { TValue *io = obj; settt_(io, BEAN_TNIL); io->prototype=NULL; }
#define setbvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).b=(x); settt_(io, BEAN_TBOOLEAN); }
#define bvalue(o)       check_exp(ttisboolean(o), val_(o).b)

#define ttisarray(o)           checktype((o), BEAN_TLIST)
#define setarrvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).ar=(x); settt_(io, BEAN_TLIST); io->prototype=G(B)->aproto; }

#define BEAN_TREGEX	(BEAN_THASH | (1 << 4))  /* regex */
#define ttishash(o)           checktag((o), BEAN_THASH)
#define ttisregex(o)           checktag((o), BEAN_TREGEX)
#define sethashvalue(obj,x)                                             \
  { TValue *io = obj; val_(io).hh=(x); settt_(io, BEAN_THASH); io->prototype=G(B)->hproto; }
#define setregexvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).rr=(x); settt_(io, BEAN_TREGEX); io->prototype=G(B)->rproto; }
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

#define nvalue(o)       check_exp(ttisnumber(o), val_(o).n)
#define setnvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).n=(x); settt_(io, x == (long)x ? BEAN_TNUMINT : BEAN_TNUMFLT); io->prototype=G(B)->nproto; }
/* }================================================================== */

#define ttisstring(o)		checktag((o), BEAN_TSTRING)
#define ttisfunction(o)		checktag((o), BEAN_TFUNCTION)
#define ttistool(o)		checktag((o), BEAN_TTOOL)
#define svalue(o)       check_exp(ttisstring(o), val_(o).s)

#define setsvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).s=(x); settt_(io, BEAN_TSTRING); io->prototype=G(B)->sproto; }

#define setfcvalue(obj,x)                                               \
  { TValue *io = obj; val_(io).fc=(x); settt_(io, BEAN_TFUNCTION); }

#define settlvalue(obj,x)                                                \
  { TValue *io = obj; val_(io).tl=(x); settt_(io, BEAN_TTOOL); }

#define fcvalue(o)       check_exp(ttisfunction(o), val_(o).fc)
#define tlvalue(o)       check_exp(ttistool(o), val_(o).tl)
#define arrvalue(o)       check_exp(ttisarray(o), val_(o).ar)
#define hhvalue(o)       check_exp(ttishash(o), val_(o).hh)
#define regexvalue(o)       check_exp(ttisregex(o), val_(o).rr)

TValue * tvalue_inspect(bean_State * B UNUSED, TValue * value);
TValue * tvalue_inspect_pure(bean_State * B UNUSED, TValue * value);
TValue * tvalue_inspect_all(bean_State * B UNUSED, TValue * value);
void add_tools(bean_State * B);
bool check_equal(TValue * v1, TValue * v2);
TValue * type_statement(bean_State * B UNUSED, TValue * target);
#endif
