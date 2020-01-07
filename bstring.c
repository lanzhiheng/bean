#include <assert.h>
#include "bstring.h"
#include "berror.h"
#include "bparser.h"
#include "utf8.h"

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
      mem_error(B, "%s", MEMERRMSG);
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
    mem_error(B, "%s", MEMERRMSG);
  }
}

static int brute_force_search_utf8(char * text, char * pattern, int n, int m){
  int ret = -1;

  if (!m) return 0; // search empty string

  int i, j, count;

  for (i = 0, count = 0; i < n; u8_nextchar(text, &i), count++) {
    int k = i; // as a pointer

    for (j = 0; j < m && i + j < n; u8_nextchar(pattern, &j)) {
      int l = j; // as a pointer

      uint32_t charcode1 = u8_nextchar(pattern, &l);
      uint32_t charcode2 = u8_nextchar(text, &k);

      if (charcode1 != charcode2) {
        break;
      }
    }

    if (j == m) {
      ret = count;
      break;
    }
  }

  return ret;
}

static int brute_force_search(char * text, char * pattern, uint32_t n, uint32_t m){
  int ret = -1;

  if (!m) return 0; // search empty string

  uint32_t i, j;
  for(i = 0; i < n; i++) {
    for(j = 0; j < m && i + j < n; j++) {
      if(text[i + j] != pattern[j]) break;
    }
    if(j == m) {
      ret = i;
    }
  }

  return ret;
}

static TString * slice(bean_State * B, TString * origin, uint32_t start, uint32_t end) {
  char * charp = getstr(origin);
  uint32_t total = end - start;
  TString * result = beanS_newlstr(B, "", total);
  char * pointer = getstr(result);
  for (uint32_t i = start; i < end; i ++) {
    *pointer = charp[i];
    pointer++;
  }
  return result;
}

static TString * case_transform(bean_State * B, TValue * this, char begin, char diff) {
  assert(ttisstring(this));
  uint32_t total = tslen(svalue(this));

  TString * result = beanS_newlstr(B, "", total);
  char * origin = getstr(svalue(this));
  char * pointer = getstr(result);

  for (uint32_t i = 0; i < total; i++) {
    char c = origin[i];
    pointer[i] = c >= begin && c <= begin + 25 ? c + diff : c;
  }

  return result;
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

int primitive_String_equal(bean_State * B UNUSED, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc == 1);
  TValue arg1 = args[0];
  setbvalue(*ret, beanS_equal(svalue(this), svalue(&arg1)));
  return BEAN_OK;
}
int primitive_String_concat(bean_State * B, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc == 1);
  TValue arg1 = args[0];
  uint32_t argLen = tslen(svalue(&arg1));
  char * argp = getstr(svalue(&arg1));

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

  setsvalue(*ret, result);
  return BEAN_OK;
}
int primitive_String_trim(bean_State * B, TValue * this, TValue * args UNUSED, int argc UNUSED, TValue ** ret) {
  assert(ttisstring(this));
  uint32_t start = 0, end = tslen(svalue(this)) - 1;
  TString * origin = svalue(this);
  char * charp = getstr(origin);
  while (isspace(charp[start])) start++;
  while (isspace(charp[end])) end--;
  setsvalue(*ret, slice(B, origin, start, end + 1));
  return BEAN_OK;
}
int primitive_String_downcase(bean_State * B, TValue * this, TValue * args, int argc, TValue ** ret) {
  setsvalue(*ret, case_transform(B, this, 'A', 'a' - 'A'));
  return BEAN_OK;
}
int primitive_String_upcase(bean_State * B, TValue * this, TValue * args UNUSED, int argc UNUSED, TValue ** ret) {
  setsvalue(*ret, case_transform(B, this, 'a', 'A' - 'a'));
  return BEAN_OK;
}
int primitive_String_capitalize(bean_State * B, TValue * this, TValue * args, int argc UNUSED, TValue ** ret) {
  TString * ts = case_transform(B, this, 'A', 0);
  char * c = getstr(ts);
  char diff = 'A' - 'a';
  if (c[0] >= 'a' && c[0] <= 'z') c[0] += diff;
  setsvalue(*ret, ts);
  return BEAN_OK;
}
int primitive_String_codePoint(bean_State * B UNUSED, TValue * this, TValue * args, int argc UNUSED, TValue ** ret) {
  assert(ttisstring(this));
  TString * ts = svalue(this);
  char * string = getstr(ts);
  int i = 0;
  uint32_t m = u8_nextchar(string, &i);
  setivalue(*ret, m);
  return BEAN_OK;
}
int primitive_String_length(bean_State * B UNUSED, TValue * this, TValue * args UNUSED, int argc UNUSED, TValue ** ret) {
  assert(ttisstring(this));
  TString * ts = svalue(this);
  char * string = getstr(ts);
  setivalue(*ret, u8_strlen(string));
  return BEAN_OK;
}
int primitive_String_indexOf(bean_State * B UNUSED, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc == 1);
  TValue pattern = args[0];
  assert(ttisstring(&pattern));
  TString * t = svalue(this);
  TString * p = svalue(&pattern);
  int iVal = brute_force_search_utf8(getstr(t), getstr(p), tslen(t), tslen(p));
  setivalue(*ret, iVal);
  return BEAN_OK;
}
int primitive_String_includes(bean_State * B UNUSED, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc == 1);
  TValue pattern = args[0];
  assert(ttisstring(&pattern));
  TString * t = svalue(this);
  TString * p = svalue(&pattern);
  int iVal = brute_force_search_utf8(getstr(t), getstr(p), tslen(t), tslen(p));
  setbvalue(*ret, iVal == -1 ? false : true);
  return BEAN_OK;
}
int primitive_String_split(bean_State * B UNUSED, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc < 2);
  Array * arr = init_array(B);
  TString * ts = svalue(this);
  int len = tslen(ts);
  char * res = getstr(ts);
  TValue delimiter;

  if (argc) {
    delimiter = args[0];
    char * ds = getstr(svalue(&delimiter));
    uint32_t dslen = tslen(svalue(&delimiter));

    while (len > 0) {
      int iVal = brute_force_search(res, ds, len, dslen);
      if (iVal == -1) iVal = len; // For the end

      if (dslen == 0) u8_nextchar(res, &iVal);

      if (iVal > 0) {
        TValue * newObj = malloc(sizeof(TValue));
        TString * newStr = beanS_newlstr(B, res, iVal);
        setsvalue(newObj, newStr);
        array_push(B, arr, newObj);
      }
      res = res + iVal + dslen;
      len = len - dslen - iVal;
    }
  } else {
    TValue * str = malloc(sizeof(TValue));
    setsvalue(str, ts);
    array_push(B, arr, str);
  }

  setarrvalue(*ret, arr);
  return BEAN_OK;
}
int primitive_String_slice(bean_State * B, TValue * this, TValue * args, int argc, TValue ** ret) {
  assert(ttisstring(this));
  assert(argc);
  if (argc != 2) {
    eval_error(B, "%s", "You must pass two arguments to call the slice method.");
  }

  TValue sVal = args[0];
  TValue eVal = args[1];

  if (!ttisinteger(&sVal)) {
    eval_error(B, "%s", "The first arguments must be integer");
  }

  if (!ttisinteger(&eVal)) {
    eval_error(B, "%s", "The second arguments must be integer");
  }

  int start = nvalue(&sVal);
  int end = nvalue(&eVal);
  char * string = getstr(svalue(this));
  int len = u8_strlen(string);

  if (start >= len) start = len;
  if (end >= len) end = len;

  if (start < 0) start = start + len;
  if (end < 0) end = end + len + 1;

  assert(start <= end);

  int startIndex = 0;
  int endIndex = 0;

  for (int i = 0; i < start; i++) {
    u8_nextchar(string, &startIndex);
  }

  for (int j = 0; j < end; j++) {
    u8_nextchar(string, &endIndex);
  }

  setsvalue(*ret, slice(B, svalue(this), startIndex, endIndex));
  return BEAN_OK;
}
