#include "bstring.h"

static TString *createstrobj (bean_State * B, size_t l, int tag, unsigned int h) {
  size_t totalsize = sizeofstring(l);
  GCObject * gc = beanC_newobj(B, tag, totalsize);
  TString *ts = gco2ts(gc);
  ts->hash = h;
  ts->reserved = 0;
  getstr(ts)[l] = '\0';
  return ts;
}
unsigned int beanS_hash (const char *str, size_t l, unsigned int seed) {
  unsigned int h = seed ^ cast_uint(l);
  size_t step = (l >> HASHLIMIT) + 1;
  for (; l >= step; l -= step)
    h ^= ((h<<5) + (h>>2) + cast_byte(str[l - 1]));
  return h;
}

TString * beanS_newlstr (bean_State * B, const char *str, size_t l) {
  unsigned int h = beanS_hash(str, l, g->seed);
  TString * ts = createstrobj(B, l, BEAN_TSTRING, h);
  memcpy(getstr(ts), str, l);
  return ts;
}
