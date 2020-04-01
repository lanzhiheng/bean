#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "vm.h"
#include "bmeta.h"
#include "bstring.h"
#include "bnumber.h"
#include "bfunction.h"
#include "bmath.h"
#include "bdate.h"
#include "bboolean.h"
/* #include "bhttp.h" */
#include "berror.h"
#include "barray.h"
#include "bregex.h"
#include "bobject.h"
#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "mem.h"

#include <setjmp.h>
jmp_buf buf;
int REPL = false;

#define MIN_STRT_SIZE 64

char * rootDir = NULL;

static stringtable stringtable_init(bean_State * B UNUSED) {
  stringtable tb;
  tb.nuse = 0;
  tb.size = MIN_STRT_SIZE;
  tb.hash = beanM_mallocvchar(B, MIN_STRT_SIZE, TString *);

  for (int i = 0; i < MIN_STRT_SIZE; i++) {
    tb.hash[i] = NULL;
  }
  return tb;
}

static char * read_source_file(bean_State * B, const char * path) {
  FILE * file = fopen(path, "r");

    if (file == NULL) io_error(B, "%s", "Can not open file.");

  struct stat fileStat;
  stat(path, &fileStat);
  size_t fileSize = fileStat.st_size;

  /* printf("File size is: %lu.\n", fileSize); */
  char * fileContent = (char *)malloc(fileSize + 1);
  if (fileContent == NULL) {
    mem_error(B, "%s", "Could't allocate memory for reading file.");
  }

  size_t numRead = fread(fileContent, sizeof(char), fileSize, file);

  if (numRead < fileSize) io_error(B, "%s", "Could't read file");

  fileContent[fileSize] = '\0';
  fclose(file);
  return fileContent;
}

static void preload_library(bean_State * B) {
  char * array = read_source_file(B, "lib/array.bean");
  beanX_setinput(B, array, beanS_newliteral(B, "array"), *array);
  bparser(B->ls);

  char * json = read_source_file(B, "lib/json.bean");
  beanX_setinput(B, json, beanS_newliteral(B, "json"), *json);
  bparser(B->ls);
}

static struct Scope * create_scope(bean_State * B, struct Scope * previous) {
  Scope * scope = malloc(sizeof(Scope));
  scope->previous = previous;
  scope->variables = init_hash(B);
  return scope;
}

static bean_State * bean_State_init() {
  LexState * ls = malloc(sizeof(LexState));
  bean_State * B = malloc(sizeof(bean_State));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);
  B -> ls = ls;
  ls -> B = B;
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

  bean_State * B = bean_State_init();
  TString * e = beanS_newlstr(B, filename, strlen(filename));
  /* printf("Source file name is %s.\n", getstr(e)); */

  preload_library(B);
  char* source = read_source_file(B, path);
  beanX_setinput(B, source, e, *source);

  bparser(B->ls);
  executeInstruct(B);
}

void run() {
  char * source;
  size_t buffersize = 200;
  ssize_t len;
  source = (char *)malloc(buffersize * sizeof(char));
  REPL = true;

  if( source == NULL) {
    perror("Unable to allocate buffer");
    exit(1);
  }

  bean_State * B = bean_State_init();
  TString * e = beanS_newlstr(B, "REPL", 4);

  preload_library(B);

  while(true) {
    setjmp(buf);
    printf("> ");
    len = getline(&source, &buffersize, stdin);
    if (len == -1) break; //  return -1 on failure

    if (len == 1 && source[0] == '\n') continue;
    beanX_setinput(B, source, e, *source);
    TValue * value = TV_MALLOC;
    bparser_for_line(B->ls, &value);
    TValue * string = tvalue_inspect_pure(B, value);
    TString * ts = svalue(string);
    printf("=> %s\n", getstr(ts));
  }
}

static void set_prototype(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h, bool getter) {
  TValue * name = TV_MALLOC;
  setsvalue(name, beanS_newlstr(B, method, len));
  TValue * func = TV_MALLOC;
  Tool * t = initialize_tool_by_fn(fn, getter);
  settlvalue(func, t);
  hash_set(B, h, name, func);
}

Tool * initialize_tool_by_fn(primitive_Fn fn, bool getter) {
  Tool * t = malloc(sizeof(Tool));
  t -> function = fn;
  t -> context = NULL;
  t -> getter = getter;
  return t;
}

void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  set_prototype(B, method, len, fn, h, false);
}

void set_prototype_getter(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  set_prototype(B, method, len, fn, h, true);
}

TValue * init_Nil(bean_State * B UNUSED) {
  TValue * proto = TV_MALLOC;
  setnilvalue(proto);
  return proto;
}

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  G -> strt = stringtable_init(B);
  G -> callStack = NULL;
  G -> globalScope = create_scope(B, NULL);
  G -> cScope = G -> globalScope;
  B -> l_G = G;

  G -> nil = init_Nil(B);
  G -> meta = init_Meta(B);
  G -> nproto = init_Number(B);
  G -> sproto = init_String(B);
  G -> aproto = init_Array(B);
  G -> hproto = init_Hash(B);
  G -> mproto = init_Math(B);
  G -> rproto = init_Regex(B);
  G -> dproto = init_Date(B);
  G -> fproto = init_Function(B);
  G -> bproto = init_Boolean(B);
  /* G -> netproto = init_Http(B); */

  G -> nproto -> prototype = G -> meta;
  G -> sproto -> prototype = G -> meta;
  G -> aproto -> prototype = G -> meta;
  G -> hproto -> prototype = G -> meta;
  G -> mproto -> prototype = G -> meta;
  G -> rproto -> prototype = G -> meta;
  G -> dproto -> prototype = G -> meta;
  G -> fproto -> prototype = G -> meta;
  G -> bproto -> prototype = G -> meta;
  /* G -> netproto -> prototype = G -> meta; */

  G -> meta -> prototype = G->nil;

  add_tools(B); // Depend on the previous setting

  TValue * self = TV_MALLOC;
  setsvalue(self, beanS_newlstr(B, "self", 4));
  hash_set(B, G->globalScope->variables, self, G->nil);

  G->instructionStream = malloc(sizeof(Mbuffer));
  beanZ_initbuffer(G->instructionStream);

  G->linenomap = malloc(sizeof(Ibuffer));
  init_ibuffer(G->linenomap);
}

void exception() {
  REPL ? longjmp(buf, 1) : abort();
}

void assert_with_message(bool condition, char * msg) {
  if (condition) {
  } else {                                      \
    printf("%s\n", msg);
    REPL ? longjmp(buf, 1) : abort();
  }
}
