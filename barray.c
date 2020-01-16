#include <stdio.h>
#include <assert.h>
#include "mem.h"
#include "barray.h"
#include "bstate.h"
#include "bstring.h"
#include "berror.h"
#include "bparser.h"

#define ARRAY_MIN_CAPACITY 16

uint32_t get_new_size(uint32_t expectSize) {
  int factor = (expectSize - 1) / ARRAY_MIN_CAPACITY + 1;
  return factor * ARRAY_MIN_CAPACITY;
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

  if (old == NULL) {
    if (index >= arr->count) {
      uint32_t newCount = index + 1;
      for (uint32_t i = arr->count; i < newCount; i++) {
        arr->entries[i] = G(B)->nil;
      }
      arr -> count = newCount;
    } else {
      arr -> count++;
    }
  }
  arr->entries[index] = value;

  return true;
}

TValue * array_get(bean_State * B, Array * arr, uint32_t index) {
  if (index >= arr -> count) {
    runtime_error(B, "%s", "index overflow of array.");
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
    runtime_error(B, "%s", "Nothing to be pop.");
  }
  TValue * value = arr->entries[--arr->count];
  return value;
}

TValue * array_shift(bean_State * B, Array * arr) {
  if (arr->count <= 0) {
    runtime_error(B, "%s", "Nothing to be shift.");
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

TValue * primitive_Array_shift(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttisarray(this));
  assert(argc == 0);
  Array * array = arrvalue(this);
  return array_shift(B, array);
}

static TValue * primitive_Array_pop(bean_State * B, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttisarray(this));
  assert(argc == 0);
  Array * array = arrvalue(this);
  return array_pop(B, array);
}

static TValue * primitive_Array_push(bean_State * B, TValue * this, TValue * args, int argc) {
  TValue * ret = malloc(sizeof(TValue));
  assert(ttisarray(this));
  assert(argc < 2);

  Array * array = arrvalue(this);
  if (argc) {
    TValue * element = &args[0];
    array_push(B, array, element);
  }

  uint32_t count = array->count;
  setnvalue(ret, count);
  return ret;
}

static TValue * primitive_Array_unshift(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc < 2);
  TValue * ret = malloc(sizeof(TValue));

  Array * array = arrvalue(this);
  if (argc) {
    TValue * element = &args[0];
    array_unshift(B, array, element);
  }

  uint32_t count = array->count;
  setnvalue(ret, count);
  return ret;
}

static TValue * primitive_Array_join(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc < 2);
  TValue * ret = malloc(sizeof(TValue));
  Array * array = arrvalue(this);
  uint32_t total = 0;
  TString * dts;

  if (argc) {
    TValue delimiter = args[0];
    assert(ttisstring(&delimiter));
    dts = svalue(&delimiter);
  } else {
    dts = beanS_newlstr(B, ",", 1);
  }

  for (uint32_t i = 0; i < array->count; i++) {
    TValue * elm = array->entries[i];
    TString * ts = svalue(tvalue_inspect(B, elm));
    total += tslen(ts);
    total += tslen(dts);
  }

  total = total - tslen(dts);
  char * resStr = malloc(sizeof(char) * total + 1);

  uint32_t index = 0;
  for (uint32_t i = 0; i < array->count; i++) {
    TValue * elm = array->entries[i];
    TString * ts = svalue(tvalue_inspect(B, elm));
    char * cStr = getstr(ts);

    for (uint32_t j = 0; j < tslen(ts); j++) {
      resStr[index++] = cStr[j];
    }

    char * dStr = getstr(dts);
    for (uint32_t k = 0; k < tslen(dts); k++) {
      resStr[index++] = dStr[k];
    }
  }
  resStr[index] = 0;
  setsvalue(ret, beanS_newlstr(B, resStr, total));
  return ret;
}

static TValue * primitive_Array_map(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc == 1);

  TValue callback = args[0];
  TValue * ret = malloc(sizeof(TValue));
  assert(ttisfunction(&callback));

  Array * arr = arrvalue(this);
  Array * newarr = init_array(B);
  Function * f = fcvalue(&callback);
  TValue * key = malloc(sizeof(TValue));

  setsvalue(key, f->p->args[0]);

  for (uint32_t i = 0; i < arr->count; i++) {
    TValue * item = NULL;
    call_stack_create_frame(B, this);
    enter_scope(B);

    SCSV(B, key, arr->entries[i]);

    for (int m = 1; m < f->p->arity; m++) { // All extra is nil
      TValue * extra = malloc(sizeof(TValue));
      setsvalue(extra, f->p->args[m]);
      SCSV(B, extra, G(B)->nil);
    }

    set_self_before_caling(B, this);
    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      item = eval(B, ex);

      if (call_stack_peek(B)) {
        item = call_stack_peek_return(B);
        break;
      };
    }
    array_push(B, newarr, item);

    leave_scope(B);
    call_stack_restore_frame(B);
  }

  setarrvalue(ret, newarr);
  return ret;
}

static TValue * primitive_Array_reverse(bean_State * B UNUSED, TValue * this, TValue * args UNUSED, int argc) {
  assert(ttisarray(this));
  assert(argc == 0);

  Array * arr = arrvalue(this);

  for (uint32_t i = 0; i < arr->count / 2; i++) {
    TValue * temp = arr->entries[i];
    arr->entries[i] = arr->entries[arr->count-i-1];
    arr->entries[arr->count-i-1] = temp;
  }

  return this;
}

static TValue * primitive_Array_reduce(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc == 2);

  TValue callback = args[0];
  assert(ttisfunction(&callback));

  Array * arr = arrvalue(this);
  TValue * val = &args[1];
  Function * f = fcvalue(&callback);
  TValue * elem = malloc(sizeof(TValue));
  setsvalue(elem, f->p->args[0]);
  TValue * acc = malloc(sizeof(TValue));
  setsvalue(acc, f->p->args[1]);

  for (uint32_t i = 0; i < arr->count; i++) {
    call_stack_create_frame(B, this);
    enter_scope(B);

    SCSV(B, elem, arr->entries[i]);
    SCSV(B, acc, val);

    for (int m = 2; m < f->p->arity; m++) { // All extra is nil
      TValue * extra = malloc(sizeof(TValue));
      setsvalue(extra, f->p->args[m]);
      SCSV(B, extra, G(B)->nil);
    }

    set_self_before_caling(B, this);
    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      val = eval(B, ex);

      if (call_stack_peek(B)) {
        val = call_stack_peek_return(B);
        break;
      };
    }
    leave_scope(B);
    call_stack_restore_frame(B);
  }
  return val;
}

static TValue * primitive_Array_each(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc == 1);

  TValue callback = args[0];
  assert(ttisfunction(&callback));
  assert(fcvalue(&callback)->p->arity >= 1);

  Array * arr = arrvalue(this);
  Function * f = fcvalue(&callback);

  uint32_t argIdx = 0;
  TValue * index = malloc(sizeof(TValue));

  TValue * item = malloc(sizeof(TValue));
  setsvalue(item, f->p->args[argIdx++]);
  TValue * retVal = malloc(sizeof(TValue));

  TValue * idxName = NULL;
  if (f->p->arity == 2) {
    idxName = malloc(sizeof(TValue));
    setsvalue(idxName, f->p->args[argIdx++]);
  }

  for (uint32_t i = 0; i < arr->count; i++) {
    enter_scope(B);
    call_stack_create_frame(B, this);

    SCSV(B, item, arr->entries[i]);

    if (f->p->arity == 2) {
      setnvalue(index, i);
      SCSV(B, idxName, index);
    }

    for (int m = argIdx; m < f->p->arity; m++) { // All extra is nil
      TValue * extra = malloc(sizeof(TValue));
      setsvalue(extra, f->p->args[m]);
      SCSV(B, extra, G(B)->nil);
    }

    set_self_before_caling(B, this);

    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      retVal = eval(B, ex);

      if (call_stack_peek(B)) {
        retVal = call_stack_peek_return(B);
        break;
      }
    }

    leave_scope(B);
    call_stack_restore_frame(B);
  }

  return this;
}

static TValue * primitive_Array_find(bean_State * B, TValue * this, TValue * args, int argc) {
  assert(ttisarray(this));
  assert(argc == 1);

  TValue callback = args[0];
  assert(ttisfunction(&callback));
  assert(fcvalue(&callback)->p->arity >= 1);

  Array * arr = arrvalue(this);
  TValue * val = G(B)->nil;
  Function * f = fcvalue(&callback);
  TValue * key = malloc(sizeof(TValue));
  setsvalue(key, f->p->args[0]);

  TValue * retVal = NULL;
  for (uint32_t i = 0; i < arr->count; i++) {
    call_stack_create_frame(B, this);
    enter_scope(B);

    SCSV(B, key, arr->entries[i]);

    for (int m = 1; m < f->p->arity; m++) { // All extra is nil
      TValue * extra = malloc(sizeof(TValue));
      setsvalue(extra, f->p->args[m]);
      SCSV(B, extra, G(B)->nil);
    }

    set_self_before_caling(B, this);
    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      retVal = eval(B, ex);

      if (call_stack_peek(B)) {
        retVal = call_stack_peek_return(B);
        break;
      };
    }

    if (truthvalue(retVal)) {
      val = arr->entries[i];
      break;
    }
    leave_scope(B);
    call_stack_restore_frame(B);
  }

  return val;
}

TValue * init_Array(bean_State * B) {
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "join", 4, primitive_Array_join, hhvalue(proto));
  set_prototype_function(B, "push", 4, primitive_Array_push, hhvalue(proto));
  set_prototype_function(B, "pop", 3, primitive_Array_pop, hhvalue(proto));
  set_prototype_function(B, "shift", 5, primitive_Array_shift, hhvalue(proto));
  set_prototype_function(B, "unshift", 7, primitive_Array_unshift, hhvalue(proto));
  set_prototype_function(B, "reverse", 7, primitive_Array_reverse, hhvalue(proto));

  // Need callback
  set_prototype_function(B, "find", 4, primitive_Array_find, hhvalue(proto));
  set_prototype_function(B, "map", 3, primitive_Array_map, hhvalue(proto));
  set_prototype_function(B, "reduce", 6, primitive_Array_reduce, hhvalue(proto));
  set_prototype_function(B, "each", 4, primitive_Array_each, hhvalue(proto));
  return proto;
}
