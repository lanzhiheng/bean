#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "bstring.h"
#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "mem.h"
#define MIN_STRT_SIZE 8

typedef struct Scope {
  struct Scope *previous;
  Hash * variable;
} Scope;

typedef TValue* (*eval_func) (bean_State * B, struct expr * expression);
char * rootDir = NULL;

#define io_error(message) printf("%s", message)
#define mem_error(message) printf("%s", message)

static TValue * int_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMINT;
  v -> value_.i = expression -> ival;
  return v;
}

static TValue * float_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMFLT;
  v -> value_.n = expression -> nval;
  return v;
}

static TValue * boolean_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TBOOLEAN;
  v -> value_.b = expression -> bval;
  return v;
}

static TValue * binary_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  TokenType op = expression -> infix.op;
  TValue * v1 = eval(B, expression -> infix.left);
  TValue * v2 = eval(B, expression -> infix.right);
  bu_byte isfloat = ttisfloat(v1) || ttisfloat(v1);

  #define cal_statement(action) do {                      \
    if (isfloat) {                                      \
      v -> value_.n = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMFLT;                          \
    } else {                                            \
      v -> value_.i = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMINT;                          \
    }                                                   \
  } while(0)

  switch(op) {
    case(TK_ADD):
      cal_statement(add);
      break;
    case(TK_SUB):
      cal_statement(sub);
      break;
    case(TK_MUL):
      cal_statement(mul);
      break;
    case(TK_DIV):
      cal_statement(div);
      break;
    default:
      printf("need more code!");
  }

  #undef cal_statement

  return v;
}

/* static TValue * funcall_eval (bean_State * B UNUSED, struct expr * expression) {} */

eval_func fn[] = {
   int_eval,
   float_eval,
   boolean_eval,
   binary_eval
};


static stringtable stringtable_init(bean_State * B) {
  stringtable tb;
  tb.nuse = 0;
  tb.size = MIN_STRT_SIZE;
  tb.hash = beanM_mallocvchar(B, MIN_STRT_SIZE, TString *);

  for (int i = 0; i < MIN_STRT_SIZE; i++) {
    tb.hash[i] = NULL;
  }
  return tb;
}

const char *beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...) {
  char * msg = malloc(MAX_STRING_BUFFER * sizeof(char));
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(msg, MAX_STRING_BUFFER, fmt, argp); // Write the string to buffer
  va_end(argp);
  return msg;
}

static char * read_source_file(const char * path) {
  FILE * file = fopen(path, "r");

  if (file == NULL) io_error("Can not open file");

  struct stat fileStat;
  stat(path, &fileStat);
  size_t fileSize = fileStat.st_size;

  printf("File size is: %lu.\n", fileSize);
  char * fileContent = (char *)malloc(fileSize + 1);
  if (fileContent == NULL) {
    mem_error("Could't allocate memory for reading file");
  }

  size_t numRead = fread(fileContent, sizeof(char), fileSize, file);

  if (numRead < fileSize) io_error("Could't read file");

  fileContent[fileSize] = '\0';
  fclose(file);
  return fileContent;
}

static struct Scope * create_scope(bean_State * B, struct Scope * previous) {
  Scope * scope = malloc(sizeof(Scope));
  scope->previous = previous;
  scope->variable = init_hash(B);
  return scope;
}

static bean_State * bean_State_init() {
  bean_State * B = malloc(sizeof(B));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);
  return B;
}

void enter_scope(bean_State * B) {
  Scope * scope = create_scope(B, G(B) -> cScope);
  G(B) -> cScope = scope;
}

void leave_scope(bean_State * B) {
  Scope * scope = G(B) -> cScope;
  G(B) -> cScope = scope -> previous;
  free(scope);
}

void run_file(const char * path) {
  const char * lastSlash = strrchr(path, '/');
  const char * filename = path;

  if (lastSlash != NULL) {
    char * root = (char*)malloc(lastSlash - path + 2); // Allocate more char space for string '/' and '\0'
    memcpy(root, path, lastSlash - path + 1);
    root[lastSlash - path + 1] = '\0';
    rootDir = root;
    filename = lastSlash + 1;
  }

  LexState * ls = malloc(sizeof(LexState));
  bean_State * B = bean_State_init();
  ls->B = B;

  TString * e = beanS_newlstr(B, filename, strlen(filename));
  printf("Source file name is %s.\n", getstr(e));
  char* source = read_source_file(path);
  beanX_setinput(B, ls, source, e, *source);
  bparser(ls);
}

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  G -> strt = stringtable_init(B);
  G -> globalScope = create_scope(B, NULL);
  G -> cScope = create_scope(B, NULL);
  B -> l_G = G;
}

TValue * eval(bean_State * B, struct expr * expression) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression);
}
