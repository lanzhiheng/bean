#include <assert.h>
#include <regex.h>
#include "bstring.h"
#include "bobject.h"
#include "berror.h"
#include "bstate.h"

static void common_replace(char * container, char * replace, size_t * j) {
  for (size_t k = 0; k < strlen(replace); k++) {
    container[*j] = replace[k];
    (*j)++;
  }
}

// For supporting the Shorthand Character Classes https://www.regular-expressions.info/shorthand.html
static char * precompile(char * target) {
  char * a = malloc(250); // TODO: Auto reallocated

  size_t i = 0; size_t j = 0;
  size_t len = strlen(target);
  while (i < len) {
    if (target[i] == '\\') {
      char next = target[i+1];

      if (next == 'd') {
        common_replace(a, "[0-9]", &j);
        i += 2;
      } else if (next == 'w') {
        common_replace(a, "[A-Za-z0-9_]", &j);
        i += 2;
      } else if (next == 's') {
        common_replace(a, "[ \t\r\n\f]", &j);
        i += 2;
      } else if (next == 'D') {
        common_replace(a, "[^0-9]", &j);
        i += 2;
      } else if (next == 'W') {
        common_replace(a, "[^A-Za-z0-9_]", &j);
        i += 2;
      } else if (next == 'S') {
        common_replace(a, "[^ \t\r\n\f]", &j);
        i += 2;
      }
      continue;
    } else {
      a[j++] = target[i++];
    }
  }
  a[j] = '\0';
  return a;
}

TValue * init_regex(bean_State * B, char * matchStr, char * modeStr) {
  int m = REG_EXTENDED | REG_NEWLINE;
  for (unsigned i = 0; i < strlen(modeStr); i++) {
    if (modeStr[i] == 'i') m = m | REG_ICASE;
  }

  char * compiledStr = precompile(matchStr);

  regex_t r;
  int crs;
  crs = regcomp(&r, compiledStr, m);

  if (crs != 0) {
    runtime_error(B, "%s", "The regex expression is invalid.");
  }

  TValue * regex = TV_MALLOC;
  Regex * re = malloc(sizeof(Regex));
  re -> rr = r;
  re -> match = matchStr;
  re -> mode = m;
  setregexvalue(regex, re);
  return regex;
}

// Build the regex
static TValue * primitive_Regex_build(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert_with_message(this == G(B)->rproto, "This method just can call by prototype 'Regex'.");
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
  assert_with_message(argc >= 1 && ttisstring(&args[0]), "Please pass a valid string instance as parameter.");
  Regex * regex = regexvalue(this);
  regex_t r = regex -> rr;

  TValue * target = &args[0];
  char * targetStr = getstr(svalue(target));
  int ers;
  ers = regexec(&r, targetStr, 0, NULL, 0);

  return ers ? G(B)->fVal : G(B)->tVal;
}

static TValue * primitive_Regex_exec(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert_with_message(argc >= 1 && ttisstring(&args[0]), "Please pass a valid string instance as parameter.");
  Regex * regex = regexvalue(this);
  regex_t r = regex -> rr;

  TValue * target = &args[0];
  char * targetStr = getstr(svalue(target));
  size_t total = r.re_nsub + 1;
  int ers;
  regmatch_t * ptr = malloc(sizeof(regmatch_t) * total);
  ers = regexec(&r, targetStr, total, ptr, REG_NOTBOL);

  Array * array = init_array(B);
  for (size_t i = 0; i < total; i++) {
    size_t start = ptr[i].rm_so;
    size_t end = ptr[i].rm_eo;
    if (start == end) continue;

    char * str = malloc(sizeof(char) * (end - start + 1));
    size_t j;

    for (j = 0; j < end - start; j ++) {
      str[j] = targetStr[j + start];
    }
    str[j] = '\0';

    TString * ts = beanS_newlstr(B, str, end - start);
    TValue * value = TV_MALLOC;
    setsvalue(value, ts);
    array_push(B, array, value);
  }
  TValue * result = TV_MALLOC;
  setarrvalue(result, array);
  return result;
}

TValue * init_Regex(bean_State * B) {
  global_State * G = B->l_G;
  TValue * proto = TV_MALLOC;
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "test", 4, primitive_Regex_test, hhvalue(proto));
  set_prototype_function(B, "exec", 4, primitive_Regex_exec, hhvalue(proto));
  set_prototype_function(B, "build", 5, primitive_Regex_build, hhvalue(proto));

  TValue * regex = TV_MALLOC;
  setsvalue(regex, beanS_newlstr(B, "Regex", 5));
  hash_set(B, G->globalScope->variables, regex, proto);
  return proto;
}
