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
    newvect = cast(TString **, beanM_reallocvchar(B, tb -> hash, osize, nsize));

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
  global_State g = G(B);
  stringtable * tb = &(g->strb);
  unsigned int h = beanS_hash(str, l, g->seed);
  TString ** list = &tb -> hash[bmod(h, tb->size)];
  TString * ts;

  for (ts = *list; ts != NULL; ts = ts->hnext) {
    if (l == ts -> len && (memcpy(getstr(ts), str, l) == 0)) return ts;
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
