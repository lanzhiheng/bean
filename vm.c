#include "vm.h"
#include "stdio.h"
#include "bstring.h"
#include "bstate.h"

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

#define PUSH(value) (*thread->esp++ = value)
#define POP(value) (*(--thread->esp))
#define DECODE loopStart:                       \
  code = *ip++; \
  switch (code)
#define CASE(shortOp) case OP_BEAN_##shortOp
#define LOOP() goto loopStart

  DECODE {
    CASE(ADD): {
      int a = 1;
      int b = 2;
      int value = a + b;
      printf("%i\n", value);
      LOOP();
    }
    CASE(SUB): {
      int a = 1;
      int b = 2;
      int value = b - a;
      printf("%i\n", value);
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
