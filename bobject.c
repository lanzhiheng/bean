#include <assert.h>
#include "bean.h"
#include "bobject.h"
#include "barray.h"
#include "bstring.h"

bool tvalue_equal(TValue * v1, TValue * v2) {
  if (v1 == v2) return true;
  if (rawtt(v1) != rawtt(v2)) return false;

  switch(v1 -> tt_) {
    case BEAN_TNUMFLT:
    case BEAN_TNUMINT:
      return nvalue(v1) == nvalue(v2);
    case BEAN_TSTRING:
      return beanS_equal(svalue(v1), svalue(v2));
    default:
      return false;
  }
}

TValue * tvalue_inspect(bean_State * B UNUSED, TValue * value) {
  switch(value -> tt_) {
    case BEAN_TBOOLEAN: {
      bool b = bvalue(value);
      if (b) {
        printf("true");
      } else {
        printf("false");
      }
      break;
    }
    case BEAN_TNUMFLT:
      printf("%Lf", fltvalue(value));
      break;
    case BEAN_TNUMINT:
      printf("%lu", ivalue(value));
      break;
    case BEAN_TSTRING:
      printf("%s", getstr(svalue(value)));
      break;
    case BEAN_TLIST: {
      Array * arr = arrvalue(value);
      printf("[");
      for (uint32_t i = 0; i < arr->count; i++) {
        TValue * v = arr->entries[i];
        if (ttisstring(v)) printf("\"");
        tvalue_inspect(B, v);
        if (ttisstring(v)) printf("\"");
        printf(", ");
      }
      printf("\b\b]");
      break;
    }
    case BEAN_THASH: {
      Hash * hash = hhvalue(value);
      printf("{");
      for (uint32_t i = 0; i < hash->capacity; i++) {
        if (!hash->entries[i]) continue;

        Entry * e = hash->entries[i];
        while (e) {
          TValue * key = e -> key;
          TValue * value = e -> value;
          tvalue_inspect(B, key);
          printf(": ");
          if (ttisstring(value)) printf("\"");
          tvalue_inspect(B, value);
          if (ttisstring(value)) printf("\"");
          e = e -> next;
        }
        printf(", ");
      }
      printf("\b\b}");
      break;
    }
    default:
      printf("invalid value");
      break;
  }
  return value;
}

TValue * primitive_print(bean_State * B UNUSED, TValue * this UNUSED, expr * expression) {
  assert(expression -> type == EXPR_CALL);

  for (int i = 0; i < expression -> call.args -> count; i ++) {
    expr * ep = expression -> call.args -> es[i];
    TValue * tvalue = eval(B, ep);
    tvalue_inspect(B, tvalue);
    printf(" ");
  }
  printf("\n");

  TValue * tvalue = malloc(sizeof(TValue));
  setnilvalue(tvalue);
  return tvalue;
}
