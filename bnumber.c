#include <assert.h>
#include "bstring.h"
#include "berror.h"
#include "bparser.h"
#include "bnumber.h"

static TValue * primitive_Number_toFixed(bean_State * B, TValue * thisVal, TValue * args, int argc) {
  assert(ttisnumber(thisVal));
  assert(argc <= 1);
  char * buff = malloc(MAX_STRING_BUFFER * sizeof(char));
  char * template = "%%.%df";
  char * format = malloc(sizeof(char) * strlen(template));
  long bit = argc > 0 ? nvalue(&args[0]) : 2;
  assert(bit > 0);
  TValue * ret = malloc(sizeof(TValue));
  sprintf(format, template, bit);
  bean_Number value = nvalue(thisVal);

  size_t i = 1;
  size_t result = snprintf(buff, MAX_STRING_BUFFER, format, value);
  size_t size = MAX_STRING_BUFFER * i;

  while (result > size) {
    size = size * (++i);
    buff = malloc(size);
    result = snprintf(buff, size, format, value);
  }

  TString * ts = beanS_newlstr(B, buff, strlen(buff));
  setsvalue(ret, ts);
  return ret;
}

TValue * init_Number(bean_State * B) {
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "toFixed", 7, primitive_Number_toFixed, hhvalue(proto));

  return proto;
}
