#include <assert.h>
#include <regex.h>
#include "bstring.h"
#include "bobject.h"
#include "berror.h"
#include "bstate.h"

Hash * init_regex(bean_State * B, TString * matchStr) {
  Hash * regex = init_hash(B);
  TValue * key = malloc(sizeof(TValue));
  TString * ts = beanS_newliteral(B, "match");
  setsvalue(key, ts);
  TValue * match = malloc(sizeof(TValue));
  setsvalue(match, matchStr);
  hash_set(B, regex, key, match);
  return regex;
}

TValue * get_match(bean_State * B, TValue * value) {
  assert(ttisregex(value));
  Hash * regex = regexvalue(value);
  TValue * key = malloc(sizeof(TValue));
  setsvalue(key, beanS_newliteral(B, "match"));
  return hash_get(B, regex, key);
}

static TValue * primitive_Regex_test(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttisregex(this));
  assert(argc == 1);
  TValue * match = get_match(B, this);
  char * matchStr = getstr(svalue(match));
  regex_t r;
  int crs, ers;
  crs = regcomp(&r, matchStr, REG_EXTENDED);

  if (crs != 0) {
    runtime_error(B, "%s", "The regex expression is invalid.");
  }

  TValue * target = &args[0];
  char * targetStr = getstr(svalue(target));
  ers = regexec(&r, targetStr, 0, NULL, REG_NOTBOL);

  return ers ? G(B)->fVal : G(B)->tVal;
}

TValue * init_Regex(bean_State * B) {
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "test", 4, primitive_Regex_test, hhvalue(proto));

  TValue * regex = malloc(sizeof(TValue));
  setsvalue(regex, beanS_newlstr(B, "Regex", 5));
  hash_set(B, G->globalScope->variables, regex, proto);
  return proto;
}
