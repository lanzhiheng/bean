#include <assert.h>
#include "bean.h"
#include "bobject.h"
#include "bregex.h"
#include "barray.h"
#include "bstring.h"
#include "berror.h"

typedef enum {
  ORIGINAL,
  DOUBLE_QUOTE_STRING,
  DOUBLE_QUOTE_ALL
} PARSER_TYPE;

// Copy from https://github.com/cesanta/mjs/blob/master/mjs/src/mjs_json.c#L44
static const char *hex_digits = "0123456789abcdef";
static char *append_hex(char *buf, char *limit, uint8_t c) {
  if (buf < limit) *buf++ = 'u';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = hex_digits[(int) ((c >> 4) % 0xf)]; // height 4 bits
  if (buf < limit) *buf++ = hex_digits[(int) (c & 0xf)]; // low 4 bits
  return buf;
}

// Copy from https://github.com/cesanta/mjs/blob/master/mjs/src/mjs_json.c#L54
/*
 * Appends quoted s to buf. Any double quote contained in s will be escaped.
 * Returns the number of characters that would have been added,
 * like snprintf.
 * If size is zero it doesn't output anything but keeps counting.
 */
static int snquote(char *buf, size_t size, const char *s, size_t len) {
  char *limit = buf + size;
  const char *end;
  /*
   * String single character escape sequence:
   * http://www.ecma-international.org/ecma-262/6.0/index.html#table-34
   *
   * 0x8 -> \b
   * 0x9 -> \t
   * 0xa -> \n
   * 0xb -> \v
   * 0xc -> \f
   * 0xd -> \r
   */
  const char *specials = "btnvfr";
  size_t i = 0;

  i++;
  if (buf < limit) *buf++ = '"';

  for (end = s + len; s < end; s++) {
    if (*s == '"' || *s == '\\') {
      i++;
      if (buf < limit) *buf++ = '\\';
    } else if (*s >= '\b' && *s <= '\r') {
      i += 2;
      if (buf < limit) *buf++ = '\\';
      if (buf < limit) *buf++ = specials[*s - '\b'];
      continue;
    } else if ((unsigned char) *s < '\b' || (*s > '\r' && *s < ' ')) {
      i += 6 /* \uXXXX */;
      if (buf < limit) *buf++ = '\\';
      buf = append_hex(buf, limit, (uint8_t) *s);
      continue;
    }
    i++;
    if (buf < limit) *buf++ = *s;
  }

  i++;
  if (buf < limit) *buf++ = '"';

  if (buf < limit) {
    *buf = '\0';
  } else if (size != 0) {
    /*
     * There is no room for the NULL char, but the size wasn't zero, so we can
     * safely put NULL in the previous byte
     */
    *(buf - 1) = '\0';
  }
  return i;
}

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

static char * hash_inspect(bean_State * B, Hash * hash, bool wrapKey) {
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
      TValue * key = wrapKey ? tvalue_inspect_all(B, e->key) : tvalue_inspect(B, e->key);
      TValue * value = tvalue_inspect_all(B, e->value);
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
      TValue * key = wrapKey ? tvalue_inspect_all(B, e->key) : tvalue_inspect(B, e->key);
      TValue * value = tvalue_inspect_all(B, e->value);
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

static TValue * inspect(bean_State * B UNUSED, TValue * value, PARSER_TYPE mode) {
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

      if (mode >= DOUBLE_QUOTE_STRING) {
        /* Auto enlarge if space not enough */
        size_t size = MAX_STRING_BUFFER;
        size_t i = 0;
        size_t result;

        do {
          size = size * (++i);
          string = malloc(size);
          result = snquote(string, size, getstr(ts), tslen(ts));
        } while (result >= size);
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
      if (mode == DOUBLE_QUOTE_ALL) {
        string = hash_inspect(B, hhvalue(value), true);
      } else {
        string = hash_inspect(B, hhvalue(value), false);
      }
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
    case BEAN_TREGEX: {
      Regex * rr = regexvalue(value);
      string = malloc(sizeof(char) * strlen(rr->match) + 2);
      sprintf(string, "/%s/", rr->match);
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
  return inspect(B, value, ORIGINAL);
}

TValue * tvalue_inspect_all(bean_State * B UNUSED, TValue * value) { // For generate json string
  return inspect(B, value, DOUBLE_QUOTE_ALL);
}

TValue * tvalue_inspect_pure(bean_State * B UNUSED, TValue * value) {
  return inspect(B, value, DOUBLE_QUOTE_STRING);
}

static TValue * primitive_print(bean_State * B UNUSED, TValue * this UNUSED, TValue * args, int argc) {
  for (int i = 0; i < argc; i ++) {
    TValue * string = tvalue_inspect(B, args+i);
    printf("%s ", getstr(svalue(string)));
  }
  printf("\n");
  return G(B)->nil;
}

static TValue * primitive_throw(bean_State * B UNUSED, TValue * this UNUSED, TValue * args, int argc) {
  assert(argc >= 1);
  TValue * string = tvalue_inspect(B, &args[0]);

  runtime_error(B, "%s", getstr(svalue(string)));
  return G(B)->nil;
}

static TValue * primitive_assert(bean_State * B UNUSED, TValue * this UNUSED, TValue * args, int argc) {
  assert(argc >= 2);
  TValue * v1 = tvalue_inspect(B, &args[0]);
  TValue * v2 = tvalue_inspect(B, &args[1]);

  if (!check_equal(v1, v2)) {
    char * left = getstr(svalue(v1));
    char * right = getstr(svalue(v2));
    runtime_error(B, "%s not equal to %s\n", left, right);
  }
  return G(B)->nil;
}

void add_tool_by_name(bean_State * B, char * str, size_t len, primitive_Fn fn) {
  Hash * variables = B -> l_G -> globalScope -> variables;
  TValue * name = malloc(sizeof(TValue));
  TString * ts = beanS_newlstr(B, str, len);
  setsvalue(name, ts);

  TValue * func = malloc(sizeof(TValue));
  Tool * tool = initialize_tool_by_fn(fn, false);

  settlvalue(func, tool);
  hash_set(B, variables, name, func);
}

// Add some default tool functions
void add_tools(bean_State * B) {
  add_tool_by_name(B, "print", 5, primitive_print);
  add_tool_by_name(B, "throw", 5, primitive_throw);
  add_tool_by_name(B, "assert", 6, primitive_assert);
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
    default:
      typeStr = "hash";
  }

  TString * ts = beanS_newlstr(B, typeStr, strlen(typeStr));
  TValue * typeVal = malloc(sizeof(TValue));
  setsvalue(typeVal, ts);
  return typeVal;
}
