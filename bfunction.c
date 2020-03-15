#include "bfunction.h"
#include "bstring.h"

static TValue * primitive_Function_call(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  TValue * self;
  TValue * ret = G(B)->nil;
  if (argc <= 0) { self = G(B) -> nil; }
  int paramIndex = 0;
  self = &args[paramIndex++];

  Function * f = fcvalue(thisVal);
  enter_scope(B);
  call_stack_create_frame(B);

  for (int i = 0; i < f->p->arity; i++) {
    TValue * key = TV_MALLOC;
    setsvalue(key, f->p->args[i]);

    if (paramIndex < argc) {
      TValue * value = &args[paramIndex++];
      SCSV(B, key, value);
    } else {
      SCSV(B, key, G(B)->nil);
    }
  }

  set_self_before_caling(B, self);
  for (int j = 0; j < f->body->count; j++) {
    expr * ex = f->body->es[j];
    ret = eval(B, ex);

    if (call_stack_peek(B)) {
      ret = call_stack_peek_return(B);
      break;
    }
  }
  call_stack_restore_frame(B);
  leave_scope(B);
  return ret;
}

static TValue * primitive_Function_apply(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  TValue * self;
  TValue * ret = G(B)->nil;
  if (argc <= 0) { self = G(B) -> nil; }
  self = &args[0];

  Array * arr;
  if (argc == 1) {
    arr = init_array(B);
  }

  if (argc >= 2) {
    assert_with_message(ttisarray(&args[1]), "You must provide an array as second parameter.");
    arr = arrvalue(&args[1]);
  }

  Function * f = fcvalue(thisVal);
  enter_scope(B);
  call_stack_create_frame(B);

  for (int i = 0; i < f->p->arity; i++) {
    TValue * key = TV_MALLOC;
    setsvalue(key, f->p->args[i]);

    if ((uint32_t)i < arr->count) {
      TValue * value = arr->entries[i];
      SCSV(B, key, value);
    } else {
      SCSV(B, key, G(B)->nil);
    }
  }

  set_self_before_caling(B, self);
  for (int j = 0; j < f->body->count; j++) {
    expr * ex = f->body->es[j];
    ret = eval(B, ex);

    if (call_stack_peek(B)) {
      ret = call_stack_peek_return(B);
      break;
    }
  }
  call_stack_restore_frame(B);
  leave_scope(B);
  return ret;
}

TValue * init_Function(bean_State * B) {
  srand(time(0));
  global_State * G = B->l_G;
  TValue * proto = TV_MALLOC;
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "call", 4, primitive_Function_call, hhvalue(proto));
  set_prototype_function(B, "apply", 5, primitive_Function_apply, hhvalue(proto));

  TValue * function = TV_MALLOC;
  setsvalue(function, beanS_newlstr(B, "Function", 8));
  hash_set(B, G->globalScope->variables, function, proto);

  return proto;
}
