#include <assert.h>
#include <math.h>
#include <time.h>
#include "bstring.h"
#include "berror.h"
#include "bparser.h"
#include "bdate.h"

static TValue * primitive_Date_now(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  TValue * ret = malloc(sizeof(TValue));
  setnvalue(ret, time(0));
  return ret;
}

TValue * init_Date(bean_State * B) {
  srand(time(0));
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "now", 3, primitive_Date_now, hhvalue(proto));

  TValue * date = malloc(sizeof(TValue));
  setsvalue(date, beanS_newlstr(B, "Date", 4));
  hash_set(B, G->globalScope->variables, date, proto);

  return proto;
}
