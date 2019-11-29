#include <stdlib.h>
#include <assert.h>
#include "blex.h"
#include "barray.h"
#include "bstate.h"

int main() {
  bean_State * B = malloc(sizeof(B));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);
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
