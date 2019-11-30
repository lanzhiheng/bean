#ifndef BEAN_ARRAY_H
#define BEAN_ARRAY_H

#include "bobject.h"
#include "bstate.h"

#define ARRAY_MIN_CAPACITY 16

typedef struct Array {
  uint32_t count;
  uint32_t capacity;
  TValue ** entries;
} Array;

Array * init_array(bean_State * B);
bool array_set(bean_State * B, Array * arr, uint32_t index, TValue * value);
TValue * array_get(bean_State * B, Array * arr, uint32_t index);
bool array_push(bean_State * B, Array * arr, TValue * value);
TValue * array_pop(bean_State * B, Array * arr);
bool array_unshift(bean_State * B, Array * arr, TValue * value);
TValue * array_shift(bean_State * B, Array * arr);

#endif
