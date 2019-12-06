#include <assert.h>
#include "bstring.h"
#include "bparser.h"

static TString *createstrobj (bean_State * B, size_t l, int tag, unsigned int h) {
  size_t totalsize = sizeofstring(l);
  GCObject * gc = beanC_newobj(B, tag, totalsize);
  TString *ts = gco2ts(gc);
  ts->hash = h;
  ts->reserved = 0;
  getstr(ts)[l] = '\0';
  return ts;
}

static void tablerehash (TString **vect, int osize, int nsize) {
  int i;

  for (i = osize; i < nsize; i++) vect[i] = NULL;

  for (i = 0; i < osize; i++) {
    TString *p = vect[i];
    vect[i] = NULL;

    while(p) { // awesome
      TString * hnext = p->hnext;
      unsigned int h = bmod(p->hash, nsize);
      p->hnext = vect[h];
      vect[h] = p;
      p = hnext;
    }
  }
}

static void beanS_resize (bean_State *B, int nsize) {
  stringtable * tb = &G(B) -> strt;
  int osize = tb -> size;
  TString **newvect;

  if (nsize <= osize) { /* shrinking table? */
    tablerehash(tb -> hash, osize, nsize);
  } else {
    // Reallocate the memory
    newvect = beanM_reallocvchar(B, tb -> hash, osize, nsize, TString *);

    if (newvect == NULL) { // reallocate failed
      MEM_ERROR(B, MEMERRMSG);
    } else {
      tb -> hash = newvect;
      tb -> size = nsize;
      tablerehash(newvect, osize, nsize);
    }
  }
}

static void growstrtab (bean_State *B, stringtable *tb) {
  if (tb -> size <= MAXSTRTB) {
    beanS_resize(B, tb->size * 2);
  } else {
    MEM_ERROR(B, MEMERRMSG);
  }
}

unsigned int beanS_hash (const char *str, size_t l, unsigned int seed) {
  unsigned int h = seed ^ cast_uint(l);
  size_t step = (l >> HASHLIMIT) + 1;
  for (; l >= step; l -= step)
    h ^= ((h<<5) + (h>>2) + cast_byte(str[l - 1]));
  return h;
}

TString * beanS_newlstr (bean_State * B, const char *str, size_t l) {
  global_State * g = G(B);
  stringtable * tb = &(g->strt);
  unsigned int h = beanS_hash(str, l, g->seed);
  int index = bmod(h, tb->size);
  TString ** list = &tb->hash[index];
  TString * ts;

  for (ts = *list; ts != NULL; ts = ts->hnext) {
    if (l == ts -> len && (memcmp(getstr(ts), str, l) == 0)) {
      return ts;
    }
  }

  /* else must create a new string */
  if (tb->nuse >= tb->size) {  /* need to grow string table? */
    growstrtab(B, tb);
    list = &tb->hash[bmod(h, tb->size)];  /* rehash with new size */
  }

  ts = createstrobj(B, l, BEAN_TSTRING, h);
  memcpy(getstr(ts), str, l);
  ts -> len = l;
  ts -> hnext = *list;
  *list = ts;
  tb -> nuse++;
  return ts;
}

bool beanS_equal(TString * ts1, TString * ts2) {
  if (tslen(ts1) != tslen(ts2)) return false;
  return memcmp(getstr(ts1), getstr(ts2), tslen(ts2)) == 0;
}

TValue *  primitive_String_equal(bean_State * B, TValue * this, expr * expression) {
  assert(ttisstring(this));
  assert(expression -> type == EXPR_CALL);
  TValue * arg1 = eval(B, expression -> call.args -> es[0]);
  TValue * v = malloc(sizeof(TValue));
  setbvalue(v, beanS_equal(svalue(this), svalue(arg1)));
  return v;
}

TValue *  primitive_String_id(bean_State * B, TValue * this, expr * expression) {
  char id[MAX_LEN_ID];
  assert(ttisstring(this));
  assert(expression -> type == EXPR_CALL);
  TValue * v = malloc(sizeof(TValue));
  TString * ts = svalue(this);
  sprintf(id, "%p", ts);
  ts = beanS_newlstr(B, id, strlen(id));
  setsvalue(v, ts);
  return v;
}

TValue *  primitive_String_concat(bean_State * B, TValue * this, expr * expression) {
  assert(ttisstring(this));
  assert(expression -> type == EXPR_CALL);
  TValue * arg1 = eval(B, expression -> call.args -> es[0]);
  uint32_t argLen = tslen(svalue(arg1));
  char * argp = getstr(svalue(arg1));

  uint32_t thisLen = tslen(svalue(this));
  char * thisp = getstr(svalue(this));

  uint32_t total = thisLen + argLen;
  TString * result = beanS_newlstr(B, "", total);
  char * pointer = getstr(result);

  uint32_t i = 0;
  for (i = 0; i < thisLen; i ++) {
    pointer[i] = thisp[i];
  }

  for (uint32_t j = 0; j < argLen; j ++) {
    pointer[i++] = argp[j];
  }
  TValue * r = malloc(sizeof(TValue));
  setsvalue(r, result);
  return r;
}
