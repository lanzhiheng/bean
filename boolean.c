#include "bboolean.h"
#include "bstring.h"

TValue * init_Boolean(bean_State * B) {
  global_State * G = B->l_G;
  TValue * proto = TV_MALLOC;
  Hash * h = init_hash(B);

  sethashvalue(proto, h);

  G -> tVal = TV_MALLOC;
  setbvalue(G->tVal, true);
  G -> tVal -> prototype = proto;
  G -> fVal = TV_MALLOC;
  setbvalue(G->fVal, false);
  G -> fVal -> prototype = proto;

  TValue * boolean = TV_MALLOC;
  setsvalue(boolean, beanS_newlstr(B, "Boolean", 7));
  hash_set(B, G->globalScope->variables, boolean, proto);
  return proto;
}
