#include <assert.h>
#include "mem.h"
#include "bmeta.h"
#include "berror.h"
#include "bstring.h"
#include "bparser.h"
#include "stdio.h"

static TValue * primitive_Meta_proto(bean_State * B UNUSED, TValue * this, TValue * args UNUSED, int argc UNUSED) {
  return this->prototype;
}

static TValue * primitive_Meta_toString(bean_State * B, TValue * this, TValue * args UNUSED, int argc UNUSED) {
  TValue * string = tvalue_inspect_pure(B, this);
  return string;
}

static TValue * primitive_Meta_id(bean_State * B, TValue * this, TValue * args UNUSED, int argc UNUSED) {
  char id[MAX_LEN_ID];
  TString * ts = NULL;
  TValue * ret = malloc(sizeof(TValue));

  switch(this->tt_) {
    case(BEAN_TSTRING): {
      TString * ss = svalue(this);
      sprintf(id, "%p", ss);
      break;
    }
    case(BEAN_THASH): {
      Hash * hash = hhvalue(this);
      sprintf(id, "%p", hash);
      break;
    }
    case(BEAN_TLIST): {
      Array * arr = arrvalue(this);
      sprintf(id, "%p", arr);
      break;
    }
    default: {
      sprintf(id, "%p", NULL);
    }
  }

  ts = beanS_newlstr(B, id, strlen(id));
  setsvalue(ret, ts);
  return ret;
}

TValue * init_Meta(bean_State * B) {
  global_State * G = B->l_G;

  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "id", 2, primitive_Meta_id, hhvalue(proto));
  set_prototype_function(B, "toString", 8, primitive_Meta_toString, hhvalue(proto));
  set_prototype_getter(B, "__proto__", 9, primitive_Meta_proto, hhvalue(proto));

  TValue * hash = malloc(sizeof(TValue));
  setsvalue(hash, beanS_newlstr(B, "Meta", 4));
  hash_set(B, G->globalScope->variables, hash, proto);
  return proto;
}
