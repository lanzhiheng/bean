#include <assert.h>
#include <regex.h>
#include "bstring.h"
#include "bobject.h"
#include "berror.h"
#include "bstate.h"

TValue * init_regex(bean_State * B, char * matchStr, char * modeStr) {
  int m = REG_EXTENDED;
  for (unsigned i = 0; i < strlen(modeStr); i++) {
    if (modeStr[i] == 'i') m = m | REG_ICASE;
  }

  regex_t r;
  int crs;
  crs = regcomp(&r, matchStr, m);

  if (crs != 0) {
    runtime_error(B, "%s", "The regex expression is invalid.");
  }

  TValue * regex = malloc(sizeof(TValue));
  Regex * re = malloc(sizeof(Regex));
  re -> rr = r;
  re -> match = matchStr;
  re -> mode = m;
  setregexvalue(regex, re);
  return regex;
}

// Build the regex
static TValue * primitive_Regex_build(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttishash(this));
  assert(argc >= 1);
  TValue * match = &args[0];
  assert(ttisstring(match));

  char * matchStr = getstr(svalue(match));

  // get mode str
  char * modeStr = "";

  if (argc >= 2) {
    TValue * mode = &args[1];
    assert(ttisstring(match));
    modeStr = getstr(svalue(mode));
  }

  return init_regex(B, matchStr, modeStr);
}

static TValue * primitive_Regex_test(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttisregex(this));
  assert(argc == 1);
  Regex * regex = regexvalue(this);
  regex_t r = regex -> rr;

  TValue * target = &args[0];
  char * targetStr = getstr(svalue(target));
  int ers;
  ers = regexec(&r, targetStr, 0, NULL, REG_NOTBOL);

  return ers ? G(B)->fVal : G(B)->tVal;
}

TValue * init_Regex(bean_State * B) {
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "test", 4, primitive_Regex_test, hhvalue(proto));
  set_prototype_function(B, "build", 5, primitive_Regex_build, hhvalue(proto));

  TValue * regex = malloc(sizeof(TValue));
  setsvalue(regex, beanS_newlstr(B, "Regex", 5));
  hash_set(B, G->globalScope->variables, regex, proto);
  return proto;
}
