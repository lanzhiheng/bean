#ifndef _BEAN_STATE_H
#define _BEAN_STATE_H

#include "common.h"
#include "bobject.h"

typedef struct stringtable {
  struct TString ** hash;
  int nuse;
  int size;
} stringtable;

typedef struct global_State {
  unsigned int seed;  /* randomized seed for hashes */
  GCObject *allgc;  /* list of all collectable objects */
  stringtable strt;
} global_State;

#define G(B)	(B->l_G)

/*
** Union of all collectable objects (only for conversions)
*/
union GCUnion {
  GCObject gc;  /* common header */
  struct TString ts;
  /* struct Udata u; */
  /* union Closure cl; */
  /* struct Table h; */
  /* struct Proto p; */
  /* struct lua_State th;  /\* thread *\/ */
  /* struct UpVal upv; */
};

typedef struct bean_State {
  size_t allocateBytes;
  global_State *l_G;
} bean_State;

#define cast_u(o)	cast(union GCUnion *, (o))

/* macros to convert a GCObject into a specific value */
#define gco2ts(o)  \
        check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))

void global_init(bean_State * B);
const char *beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...);
#endif
