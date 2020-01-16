#include <assert.h>
#include "bean.h"
#include "bobject.h"
#include "barray.h"
#include "bstring.h"

bool check_equal(TValue * v1, TValue * v2) {
  if (rawtt(v1) != rawtt(v2)) return false;

  if (ttisnumber(v1) && ttisnumber(v2)) {
    return nvalue(v1) == nvalue(v2);
  } else if (ttisstring(v1) && ttisstring(v2)) {
    return beanS_equal(svalue(v1), svalue(v2));
  } else if (v1 == v2) {
    return true;
  }
  return false;
}

static char * hash_inspect(bean_State * B, Hash * hash) {
  uint32_t total = 0;
  char * colon = ": ";
  char * comma = ", ";
  uint32_t colon_Len = strlen(colon);
  uint32_t comma_Len = strlen(comma);

  if (!hash->count) return "{}";

  for (uint32_t i = 0; i < hash->capacity; i++) {
    if (!hash->entries[i]) continue;

    Entry * e = hash->entries[i];
    while (e) {
      TValue * key = tvalue_inspect(B, e->key);
      TValue * value = tvalue_inspect_pure(B, e->value);
      total += tslen(svalue(key)) + tslen(svalue(value)) + colon_Len + comma_Len;
      e = e -> next;
    }
  }
  total = total - comma_Len;

  char * resStr = malloc(sizeof(char) * total + 2 + 1);

  uint32_t index = 0;
  resStr[index++] = '{';
  for (uint32_t i = 0; i < hash->capacity; i++) {
    if (!hash->entries[i]) continue;

    Entry * e = hash->entries[i];
    while (e) {
      TValue * key = tvalue_inspect(B, e->key);
      TValue * value = tvalue_inspect_pure(B, e->value);
      char * key_Str = getstr(svalue(key));
      char * value_Str = getstr(svalue(value));

      for (uint32_t j = 0; j < strlen(key_Str); j++) {
        resStr[index++] = key_Str[j];
      }

      for (uint32_t k = 0; k < colon_Len; k++) {
        resStr[index++] = colon[k];
      }

      for (uint32_t l = 0; l < strlen(value_Str); l++) {
        resStr[index++] = value_Str[l];
      }

      for (uint32_t m = 0; m < comma_Len; m++) {
        resStr[index++] = comma[m];
      }

      e = e -> next;
    }
  }

  index -= comma_Len;
  resStr[index++] = '}';
  resStr[index] = 0;
  return resStr;
}

static char * array_inspect(bean_State * B, Array * array) {
  uint32_t total = 0;
  char * delimiter = ", ";
  uint32_t del_Len = strlen(delimiter);

  if (!array->count) return "[]";

  for (uint32_t i = 0; i < array->count; i++) {
    TValue * elm = array->entries[i];
    TString * ts = svalue(tvalue_inspect_pure(B, elm));
    total += tslen(ts);
    total += del_Len;
  }

  total = total - del_Len;

  char * resStr = malloc(sizeof(char) * total + 2 + 1);
  uint32_t index = 0;

  resStr[index++] = '[';
  for (uint32_t i = 0; i < array->count; i++) {
    TValue * elm = array->entries[i];
    TString * ts = svalue(tvalue_inspect_pure(B, elm));
    char * cStr = getstr(ts);

    for (uint32_t j = 0; j < tslen(ts); j++) {
      resStr[index++] = cStr[j];
    }

    for (uint32_t k = 0; k < del_Len; k++) {
      resStr[index++] = delimiter[k];
    }
  }
  index = index - del_Len;
  resStr[index++] = ']';
  resStr[index] = 0;
  return resStr;
}

static TValue * inspect(bean_State * B UNUSED, TValue * value, bool pure) {
  TValue * inspect = malloc(sizeof(TValue));
  char * string;

  switch(value -> tt_) {
    case BEAN_TNIL: {
      string = "nil";
      break;
    }
    case BEAN_TBOOLEAN: {
      bool b = bvalue(value);
      string = b ? "true" : "false";
      break;
    }
    case BEAN_TNUMFLT:
      string = malloc(sizeof(char) * 100);
      sprintf(string, "%f", nvalue(value));
      break;
    case BEAN_TNUMINT:
      string = malloc(sizeof(char) * 100);
      sprintf(string, "%ld", (long)nvalue(value));
      break;
    case BEAN_TSTRING: {
      TString * ts = svalue(value);
      uint32_t len = tslen(ts);
      if (pure) {
        string = malloc(sizeof(char) * len + 2 + 1);
        sprintf(string, "\"%s\"", getstr(ts));
      } else {
        string = malloc(sizeof(char) * len + 1);
        sprintf(string, "%s", getstr(ts));
      }
      break;
    }
    case BEAN_TLIST: {
      string = array_inspect(B, arrvalue(value));
      break;
    }
    case BEAN_THASH: {
      string = hash_inspect(B, hhvalue(value));
      break;
    }
    case BEAN_TFUNCTION: {
      Function * f = fcvalue(value);
      TString * ts = f->p->name;
      uint32_t len = tslen(ts);
      string = malloc(sizeof(char) * len + 1 + 11);
      sprintf(string, "[Function %s]", getstr(ts));
      break;
    }
    case BEAN_TTOOL: {
      Tool * t = tlvalue(value);
      string = t->getter ? "[Primitive getter]" : "[Primitive function]";
      break;
    }
    default:
      string = "invalid value";
      break;
  }
  TString * ts = beanS_newlstr(B, string, strlen(string));
  setsvalue(inspect, ts);
  return inspect;
}

TValue * tvalue_inspect(bean_State * B UNUSED, TValue * value) {
  return inspect(B, value, false);
}

TValue * tvalue_inspect_pure(bean_State * B UNUSED, TValue * value) {
  return inspect(B, value, true);
}

static TValue * primitive_print(bean_State * B UNUSED, TValue * this UNUSED, TValue * args, int argc) {
  for (int i = 0; i < argc; i ++) {
    TValue * string = tvalue_inspect(B, args+i);
    printf("%s ", getstr(svalue(string)));
  }
  printf("\n");
  return G(B)->nil;
}

// Add some default tool functions
void add_tools(bean_State * B) {
  Hash * variables = B -> l_G -> globalScope -> variables;
  TValue * name = malloc(sizeof(TValue));
  TString * ts = beanS_newlstr(B, "print", 5);
  setsvalue(name, ts);

  TValue * func = malloc(sizeof(TValue));
  Tool * tool = initialize_tool_by_fn(primitive_print, false);

  settlvalue(func, tool);
  hash_set(B, variables, name, func);
}

TValue * type_statement(bean_State * B UNUSED, TValue * target) {
  char * typeStr;
  switch(ttype(target)) {
    case BEAN_TNIL:
      typeStr = "nil";
      break;
    case BEAN_TBOOLEAN:
      typeStr = "boolean";
      break;
    case BEAN_TNUMBER:
      typeStr = "number";
      break;
    case BEAN_TTOOL:
    case BEAN_TFUNCTION:
      typeStr = "function";
      break;
    case BEAN_TSTRING:
      typeStr = "string";
      break;
    case BEAN_TLIST:
      typeStr = "array";
      break;
  }

  TString * ts = beanS_newlstr(B, typeStr, strlen(typeStr));
  TValue * typeVal = malloc(sizeof(TValue));
  setsvalue(typeVal, ts);
  return typeVal;
}
