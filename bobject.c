#include "bobject.h"
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
    case BEAN_TNUMFLT:
      printf("%Lf", fltvalue(value));
      break;
    case BEAN_TNUMINT:
      printf("%lu", ivalue(value));
      break;
    case BEAN_TSTRING:
      printf("%s", getstr(svalue(value)));
      break;
    default:
      printf("invalid value\n");
      break;
  }
  printf("\n");
  return value;
}
