#include "mem.h"
#include "hash.h"
#include "bstring.h"
#include "stdio.h"

#define FACTOR 0.75
#define ENLARGE_FACTOR 2

typedef union {
  uint64_t bits64;
  uint32_t bits32[2];
  double num;
} Bits64;

Hash * init_hash(bean_State * B) {
  Hash * hash = beanM_malloc_(B, Hash);
  hash -> count = 0;
  hash -> capacity = 0;
  hash -> entries = NULL;
  return hash;
}

uint32_t hash_string(char * str, uint32_t length) {
  uint32_t hashCode = 2166136261, idx= 0;
  while (idx < length) {
    hashCode ^= str[idx];
    hashCode *= 16777619;
    idx++;
  }

  return hashCode;
}

static uint32_t hash_num(double num) {
  Bits64 bits64;
  bits64.num = num;
  return bits64.bits32[0] ^ bits64.bits32[1];
}

static uint32_t hash_obj(TValue * key, uint32_t size) {
  switch (key -> tt_) {
    case BEAN_TNUMFLT:
    case BEAN_TNUMINT:
      return hash_num(nvalue(key)) % size;
    case BEAN_TSTRING: {
      TString * t = svalue(key);
      return hash_string(getstr(t), tslen(t)) % size;
    }
    default:
      printf("%s\n", "Invalid key type");
  }
  return 0;
}

static void hash_resize(bean_State * B, Hash * hash, int newCapacity) {
  Entry ** new_entries = beanM_array_malloc_(B, Entry *, newCapacity);

  if (hash -> capacity > 0) {
    for (uint32_t i = 0; i < hash -> capacity; i++) {
      Entry * entry = hash -> entries[i];

      while (entry) { // TODO: need test
        Entry * next = entry -> next;
        uint32_t new_p = hash_obj(entry -> key, newCapacity);
        entry -> next = new_entries[new_p];
        new_entries[new_p] = entry;
        entry = next;
      }
    }
  }

  beanM_array_dealloc_(B, hash -> entries, hash -> capacity);
  hash -> entries = new_entries;
  hash -> capacity = newCapacity;
}

bool hash_set(bean_State * B, Hash * hash, TValue * key, TValue * value) {
  if (hash -> capacity * FACTOR <= hash -> count) {
    int newCapacity = hash -> capacity * ENLARGE_FACTOR;
    newCapacity = newCapacity > HASH_MIN_CAPACITY ? newCapacity : HASH_MIN_CAPACITY;
    hash_resize(B, hash, newCapacity);
  }
  uint32_t index = hash_obj(key, hash -> capacity);

  Entry * entry = beanM_malloc_(B, Entry);
  entry -> key = key;
  entry -> value = value;
  entry -> next = hash -> entries[index];
  hash -> entries[index] = entry;
  return true;
}

TValue * hash_get(bean_State * B, Hash * hash, TValue * key) {
  uint32_t index = hash_obj(key, hash -> capacity);
  Entry * entry = hash -> entries[index];

  while (entry) {
    /* entry -> key  */
    entry = entry -> next;
  }
}
