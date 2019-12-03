#ifndef BEAN_HASH_H
#define BEAN_HASH_H

#include "bobject.h"
#include "bstate.h"

#define HASH_MAX_LEN_OF_KEY 255
#define HASH_MIN_CAPACITY 16

typedef struct Entry {
  TValue * key;
  TValue * value;
  struct Entry * next;
} Entry;

typedef struct Hash {
  uint32_t count;
  uint32_t capacity;
  Entry ** entries;
} Hash;

typedef struct bean_State bean_State;

Hash * init_hash(bean_State * B);
bool hash_set(bean_State * B, Hash * hash, TValue * key, TValue * value);
TValue * hash_get(bean_State * B, Hash * hash, TValue * key);
TValue * hash_remove(bean_State * B, Hash * hash, TValue * key);

#endif
