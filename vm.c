#include "vm.h"
#include "stdio.h"
#include "bstate.h"
#include "bstring.h"
#include "bstate.h"

bool operand_decode(char * buf, uint64_t *num, size_t *llen) {
  size_t i = 0, shift = 0;
  uint64_t n = 0;

  do {
    if (i == (8 * sizeof(*num) / 7 + 1)) return false;
    /*
     * Each byte of varint contains 7 bits, in little endian order.
     * MSB is a continuation bit: it tells whether next byte is used.
     */
    n |= ((uint64_t)(buf[i] & 0x7f)) << shift;
    /*
     * First we increment i, then check whether it is within boundary and
     * whether decoded byte had continuation bit set.
     */
    i++;
    shift += 7;
  } while (shift < sizeof(uint64_t) * 8 && (buf[i - 1] & 0x80));

  *num = n;
  *llen = i;

  return true;
}

typedef struct Thread {
  TValue ** stack;
  TValue ** esp;
} Thread;

int executeInstruct(bean_State * B) {
  Thread * thread = malloc(sizeof(Thread));
  thread -> stack = malloc(sizeof(TValue*) * 3000);
  thread -> esp = thread -> stack;

  char * ip;
  ip = G(B)->instructionStream->buffer;
  char code;

#define READ_BYTE() *ip++;
#define PUSH(value) (*thread->esp++ = value)
#define POP(value) (*(--thread->esp))
#define DECODE loopStart:                       \
  code = READ_BYTE();                         \
  switch (code)
#define CASE(shortOp) case OP_BEAN_##shortOp
#define LOOP() goto loopStart

  DECODE {
    CASE(ADD): {
      TValue * v1 = POP();
      TValue * v2 = POP();
      double a = nvalue(v1) + nvalue(v2);
      setnvalue(v1, a);
      PUSH(v1);
      free(v2);
      LOOP();
    }
    CASE(SUB): {
      int a = 1;
      int b = 2;
      int value = b - a;
      printf("%i\n", value);
      LOOP();
    }
    CASE(PUSH_NUM): {
      uint64_t n;
      size_t len;
      operand_decode(ip++, &n, &len);
      TValue * number = TV_MALLOC;
      setnvalue(number, (double)n);
      PUSH(number);
      ip += len - 1;
      LOOP();
    }
    CASE(PUSH_TRUE): {
      PUSH(G(B)->tVal);
      LOOP();
    }
    CASE(PUSH_FALSE): {
      PUSH(G(B)->fVal);
      LOOP();
    }
    CASE(PUSH_NIL): {
      PUSH(G(B)->nil);
      LOOP();
    }
    CASE(INSPECT): {
      TValue * value = POP();
      TValue * tvString = tvalue_inspect(B, value);
      char * string = getstr(svalue(tvString));
      printf("%s\n", string);
      LOOP();
    }
    default: {
      break;
    }
  }
  return 0;
}
