#include "mem.h"
#include "barray.h"

#define ARRAY_MIN_CAPACITY 16

Array * init_array(bean_State * B) {
  Array * array = beanM_malloc_(B, Array);
  array -> count = 0;
  array -> capacity = ARRAY_MIN_CAPACITY;
  array -> entries = beanM_array_malloc_(B, TValue *, ARRAY_MIN_CAPACITY);
  for (uint32_t i = 0; i < array -> capacity; i++) array->entries[i] = NULL;
  return array;
}

bool array_set(bean_State * B, Array * arr, uint32_t index, TValue * value) {
  if (index > arr->capacity) {
    uint32_t factor = index / arr->capacity + 1;
    uint32_t newSize = factor * arr->capacity;
    TValue ** newArray = beanM_array_malloc_(B, TValue *, newSize);

    for (uint32_t i = 0; i < newSize; i ++) {
      if (i < arr->capacity) {
        newArray[i] = arr->entries[i];
      } else {
        newArray[i] = NULL;
      }
    }
    beanM_array_dealloc_(B, arr->entries, arr->capacity);
    arr->capacity = newSize;
    arr->entries = newArray;
  }
  arr->entries[index] = value;
  arr->count++;
  return true;
}
