#include "vm.h"
#include "stdio.h"
#include "bstate.h"

int executeInstruct(bean_State * B) {
  char * ip;
  ip = G(B)->instructionStream->buffer;
  char code;

#define DECODE loopStart: \
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
      printf("true\n");
      LOOP();
    }
    CASE(PUSH_FALSE): {
      printf("false\n");
      LOOP();
    }
    default: {
      break;
    }
  }
  return 0;
}
