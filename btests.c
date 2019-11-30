#include "btests.h"
#include <stdlib.h>
#include <assert.h>
#include "blex.h"
#include "barray.h"
#include "bstate.h"

bean_State * get_state() {
  bean_State * B = malloc(sizeof(B));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);
  return B;
}

void test_barray() {
  bean_State * B = get_state();
  Array * array = init_array(B);
  assert(array->count == 0);
  assert(array->capacity == ARRAY_MIN_CAPACITY);
  TValue * v1 = malloc(sizeof(TValue));
  TValue * v2 = malloc(sizeof(TValue));
  setivalue(v1, 10);
  setivalue(v2, 1000);
  array_set(B, array, 0, v1);
  array_set(B, array, 0, v2);
  assert(array->count == 2);
}

void test() {
  test_barray();
}
