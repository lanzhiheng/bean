#include "bobject.h"
#include "bstring.h"

bool tvalue_equal(TValue * v1, TValue * v2) {
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
