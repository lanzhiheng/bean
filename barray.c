#include "mem.h"
#include "barray.h"
#include <stdio.h>

#define ARRAY_MIN_CAPACITY 16

uint32_t get_new_size(uint32_t expectSize) {
  int factor = (expectSize - 1) / ARRAY_MIN_CAPACITY + 1;
  return factor * ARRAY_MIN_CAPACITY;
}

void beanA_error(bean_State * B UNUSED, const char * msg) {
  printf("%s\n", msg);
  abort();
}

Array * init_array(bean_State * B) {
  Array * array = beanM_malloc_(B, Array);
  array -> count = 0;
  array -> capacity = ARRAY_MIN_CAPACITY;
  array -> entries = beanM_array_malloc_(B, TValue *, ARRAY_MIN_CAPACITY);
  for (uint32_t i = 0; i < array -> capacity; i++) array->entries[i] = NULL;
  return array;
}

static void resize_array(bean_State * B, Array * arr, uint32_t newSize) {
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

bool array_set(bean_State * B, Array * arr, uint32_t index, TValue * value) {
  if (index >= arr->capacity) {
    resize_array(B, arr, get_new_size(index + 1));
  }
  TValue * old = arr->entries[index];
  arr->entries[index] = value;

  if (old == NULL) {
    if (index >= arr->count) {
      arr -> count = index + 1;
    } else {
      arr -> count++;
    }
  }

  return true;
}

TValue * array_get(bean_State * B, Array * arr, uint32_t index) {
  if (index >= arr -> count) {
    beanA_error(B, "index overflow of array");
  }

  TValue * old = arr->entries[index];
  return old;
}

bool array_push(bean_State * B, Array * arr, TValue * value) {
  if (arr->count + 1 >= arr->capacity) {
    resize_array(B, arr, get_new_size(arr -> count + 1));
  }
  arr->entries[arr->count] = value;
  arr->count++;
  return true;
}

TValue * array_pop(bean_State * B, Array * arr) {
  if (arr->count <= 0) {
    beanA_error(B, "Nothing to be pop");
  }
  TValue * value = arr->entries[--arr->count];
  return value;
}

TValue * array_shift(bean_State * B, Array * arr) {
  if (arr->count <= 0) {
    beanA_error(B, "Nothing to be shift");
  }

  TValue * value = arr->entries[0];
  uint32_t position = 0;
  while(position < arr->count) {
    arr->entries[position] = arr->entries[position+1];
    position++;
  }

  arr->count--;
  return value;
}

bool array_unshift(bean_State * B, Array * arr, TValue * value) {
  if (arr->count + 1 >= arr->capacity) {
    resize_array(B, arr, get_new_size(arr->count + 1));
  }

  uint32_t position = arr->count;
  while(position > 0) {
    arr->entries[position] = arr->entries[position-1];
    position--;
  }
  arr->entries[position] = value;
  arr->count++;
  return true;
}
