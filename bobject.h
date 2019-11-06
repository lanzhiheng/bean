#ifndef _BEAN_OBJECT_H
#define _BEAN_OBJECT_H

#include "common.h"
#include <stdlib.h>

#define MAX_STRING_BUFFER 256

/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	struct GCObject *next; bu_byte tt; bu_byte marked

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
} Value;

/*
** Description of a local variable for function prototypes
** (used for debug information)
*/
typedef struct LocVar {
  TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;  /* first point where variable is dead */
} LocVar;

/*
** Description of an upvalue for function prototypes
*/
typedef struct Upvaldesc {
  TString *name;  /* upvalue name (for debug information) */
  bu_byte instack;  /* whether it is in stack (register) */
  bu_byte idx;  /* index of upvalue (in stack or in outer function's list) */
  bu_byte kind;  /* kind of corresponding variable */
} Upvaldesc;

#define TValuefields   Value value_; bu_byte tt_

typedef struct TValue {
  TValuefields;
} TValue;

/*
** Function Prototypes
*/
typedef struct Proto {
  CommonHeader;
  /* lu_byte numparams;  /\* number of fixed (named) parameters *\/ */
  /* lu_byte is_vararg; */
  /* lu_byte maxstacksize;  /\* number of registers needed by this function *\/ */
  /* int sizeupvalues;  /\* size of 'upvalues' *\/ */
  /* int sizek;  /\* size of 'k' *\/ */
  /* int sizecode; */
  /* int sizelineinfo; */
  /* int sizep;  /\* size of 'p' *\/ */
  int sizelocvars;
  /* int sizeabslineinfo;  /\* size of 'abslineinfo' *\/ */
  int linedefined;  /* debug information  */
  /* int lastlinedefined;  /\* debug information  *\/ */
  /* TValue *k;  /\* constants used by the function *\/ */
  /* Instruction *code;  /\* opcodes *\/ */
  /* struct Proto **p;  /\* functions defined inside the function *\/ */
  Upvaldesc *upvalues;  /* upvalue information */
  /* ls_byte *lineinfo;  /\* information about source lines (debug information) *\/ */
  /* AbsLineInfo *abslineinfo;  /\* idem *\/ */
  LocVar *locvars;  /* information about local variables (debug information) */
  /* TString  *source;  /\* used for debug information *\/ */
  /* GCObject *gclist; */
} Proto;

/*
** Some helper macro
**
*/

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
