#include <assert.h>
#include <math.h>
#include <time.h>
#include "bstring.h"
#include "berror.h"
#include "bparser.h"
#include "bmath.h"

#ifndef M_PI
define M_PI 3.14159265358979323846
#endif

#define single(action, arg) do {                \
    assert(argc >= 1);                          \
    TValue * ret = malloc(sizeof(TValue));      \
    double val = nvalue(arg);                   \
    setnvalue(ret, action(val));                \
    return ret;                                 \
  } while(0);

#define binary(action, args) do {               \
    assert(argc >= 2);                          \
    TValue * ret = malloc(sizeof(TValue));      \
    double val1 = nvalue(&args[0]);              \
    double val2 = nvalue(&args[1]);              \
    setnvalue(ret, action(val1, val2));         \
    return ret;                                 \
  } while(0);

static TValue * primitive_Math_ceil(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(ceil, &args[0]);
}

static TValue * primitive_Math_floor(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(floor, &args[0]);
}

static TValue * primitive_Math_round(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(round, &args[0]);
}

static TValue * primitive_Math_sin(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(sin, &args[0]);
}

static TValue * primitive_Math_cos(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(cos, &args[0]);
}

static TValue * primitive_Math_abs(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(fabs, &args[0]);
}

static TValue * primitive_Math_sqrt(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(sqrt, &args[0]);
}

static TValue * primitive_Math_log(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(log, &args[0]);
}

static TValue * primitive_Math_exp(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc) {
  single(exp, &args[0]);
}

static TValue * primitive_Math_random(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  TValue * ret = malloc(sizeof(TValue));
  setnvalue(ret, (double)rand() / 0x7fffffff);
  return ret;
}

static TValue * primitive_Math_max(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc UNUSED) {
  binary(fmax, args);
}

static TValue * primitive_Math_min(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc UNUSED) {
  binary(fmin, args);
}

static TValue * primitive_Math_pow(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc UNUSED) {
  binary(pow, args);
}

TValue * init_Math(bean_State * B) {
  srand(time(0));
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "ceil", 4, primitive_Math_ceil, hhvalue(proto));
  set_prototype_function(B, "floor", 5, primitive_Math_floor, hhvalue(proto));
  set_prototype_function(B, "round", 5, primitive_Math_round, hhvalue(proto));
  set_prototype_function(B, "sin", 3, primitive_Math_sin, hhvalue(proto));
  set_prototype_function(B, "cos", 3, primitive_Math_cos, hhvalue(proto));
  set_prototype_function(B, "abs", 3, primitive_Math_abs, hhvalue(proto));
  set_prototype_function(B, "sqrt", 4, primitive_Math_sqrt, hhvalue(proto));

  set_prototype_function(B, "log", 3, primitive_Math_log, hhvalue(proto));
  set_prototype_function(B, "exp", 3, primitive_Math_exp, hhvalue(proto));
  set_prototype_function(B, "random", 6, primitive_Math_random, hhvalue(proto));

  set_prototype_function(B, "min", 3, primitive_Math_min, hhvalue(proto));
  set_prototype_function(B, "max", 3, primitive_Math_max, hhvalue(proto));
  set_prototype_function(B, "pow", 3, primitive_Math_pow, hhvalue(proto));

  TValue * pi = malloc(sizeof(TValue));
  setsvalue(pi, beanS_newlstr(B, "PI", 2));
  TValue * pi_v = malloc(sizeof(TValue));
  setnvalue(pi_v, M_PI);
  hash_set(B, hhvalue(proto), pi, pi_v);

  TValue * math = malloc(sizeof(TValue));
  setsvalue(math, beanS_newlstr(B, "Math", 4));
  hash_set(B, G->globalScope->variables, math, proto);

  return proto;
}
