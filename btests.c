#include "btests.h"
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include "blex.h"
#include "barray.h"
#include "bstring.h"
#include "bhash.h"
#include "bstate.h"
#include "bparser.h"
#include "math.h"

bean_State * get_state() {
  bean_State * B = malloc(sizeof(B));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);
  return B;
}

void test_barray_pop_and_push(bean_State * B) {
  Array * array = init_array(B);
  TValue * v1 = malloc(sizeof(TValue));
  TValue * v2 = malloc(sizeof(TValue));
  setivalue(v1, 100);
  setivalue(v2, 300);
  array_push(B, array, v1);
  array_push(B, array, v2);
  assert(check_equal(array_get(B, array, 0), v1));
  assert(check_equal(array_get(B, array, 1), v2));
  assert(array->count == 2);
  assert(array->capacity == ARRAY_MIN_CAPACITY);

  TValue * popValue1 = array_pop(B, array);
  TValue * popValue2 = array_pop(B, array);
  assert(check_equal(popValue1, v2));
  assert(check_equal(popValue2, v1));
  assert(array->count == 0);
}

void test_barray_shift_and_unshift(bean_State * B) {
  Array * array = init_array(B);
  TValue * v1 = malloc(sizeof(TValue));
  TValue * v2 = malloc(sizeof(TValue));
  TValue * v3 = malloc(sizeof(TValue));
  array_unshift(B, array, v1);
  array_unshift(B, array, v2);
  array_unshift(B, array, v3);
  assert(check_equal(array_get(B, array, 0), v3));
  assert(check_equal(array_get(B, array, 1), v2));
  assert(check_equal(array_get(B, array, 2), v1));
  assert(array->count == 3);

  TValue * shiftValue1 = array_shift(B, array);
  TValue * shiftValue2 = array_shift(B, array);
  TValue * shiftValue3 = array_shift(B, array);
  assert(check_equal(shiftValue1, v3));
  assert(check_equal(shiftValue2, v2));
  assert(check_equal(shiftValue3, v1));
  assert(array->count == 0);
}

void test_barray_set_value(bean_State * B) {
  Array * array = init_array(B);
  TValue * v1 = malloc(sizeof(TValue));
  TValue * v2 = malloc(sizeof(TValue));
  TValue * v3 = malloc(sizeof(TValue));

  array_set(B, array, 10, v1);
  assert(array->count == 11);
  assert(array->capacity == 16);
  array_set(B, array, 60, v2);
  assert(array->count == 61);
  assert(array->capacity == 64);
  array_set(B, array, 80, v3);
  assert(array->count == 81);
  assert(array->capacity == 96);
  assert(check_equal(array_get(B, array, 80), v3));
  assert(check_equal(array_get(B, array, 60), v2));
  assert(check_equal(array_get(B, array, 10), v1));
}

static TValue * getValue(bean_State * B, uint32_t i) {
  TValue * value = malloc(sizeof(TValue));
  setivalue(value, i);
  return value;
}

static TValue * getKey(bean_State * B, uint32_t i) {
  TValue * key = malloc(sizeof(TValue));
  char keyBuffer[HASH_MAX_LEN_OF_KEY];
  sprintf(keyBuffer, "key-%d", i);
  TString * sKey = beanS_newlstr(B, keyBuffer, strlen(keyBuffer));
  setsvalue(key, sKey);
  return key;
}

static void testing_value_by_count (bean_State * B, Hash * hash, uint32_t count) {

  for (uint32_t i = 0; i < count; i ++) {
    TValue * key = getKey(B, i);
    TValue * value = getValue(B, i);
    hash_set(B, hash, key, value);
  }

  for (uint32_t i = 0; i < count; i ++) {
    TValue * key = getKey(B, i);
    TValue * value = getValue(B, i);
    assert(check_equal(hash_get(B, hash, key), value));
  }
}

void test_hash_set(bean_State * B) {
  Hash * hash = init_hash(B);

  assert(hash->count == 0);
  assert(hash->capacity == 16);

  testing_value_by_count(B, hash, 10);

  assert(hash->count == 10);
  assert(hash->capacity == 16);

  testing_value_by_count(B, hash, 100);

  assert(hash->count == 100);
  assert(hash->capacity == 256);
}

void test_bhash() {
  bean_State * B = get_state();
  test_hash_set(B);
  printf("==============================\n");
  printf("All tests of hash passed!\n");
  printf("==============================\n");
}

void test_barray() {
  bean_State * B = get_state();
  test_barray_pop_and_push(B);
  test_barray_shift_and_unshift(B);
  test_barray_set_value(B);

  printf("==============================\n");
  printf("All tests of array passed!\n");
  printf("==============================\n");
}

void test_tokens() {
  uint32_t size = TK_STRING - TK_AND + 1;
  for (uint32_t i = 0; i < size; i++) {
    assert(memcmp(symbol_table[i].id, beanX_tokens[i], strlen(symbol_table[i].id)) == 0);
  }
  printf("Length of tokens area %d.\n", size);
}

void test() {
  test_barray();
  test_bhash();
  test_tokens();
}
