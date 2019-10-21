#include "lgc.h"
#include "mem.h"

/*
** create a new collectable object (with given type and size) and link
** it to 'allgc' list.
*/

GCObject *beanC_newobj (bean_State *B, int tt, size_t sz) {
  global_State *g = G(B);
  GCObject *o = cast(GCObject *, beanM_newobject(B, novariant(tt), sz));
  /* o->marked = luaC_white(g); */
  o->tt = tt;
  o->next = g->allgc;
  g->allgc = o;
  return o;
}
