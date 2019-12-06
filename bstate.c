#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "bstring.h"
#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "mem.h"
#define MIN_STRT_SIZE 8

typedef TValue* (*eval_func) (bean_State * B, struct expr * expression);
char * rootDir = NULL;

#define io_error(message) printf("%s", message)
#define mem_error(message) printf("%s", message)
#define eval_error(message) ({                  \
      printf("%s\n", message);                    \
      abort();                                  \
})

static TValue * get_callee_name(bean_State * B UNUSED, expr * expression) {
  TString * callee = expression -> call.callee;
  TValue * mname = malloc(sizeof(TValue));
  setsvalue(mname, callee);
  return mname;
}

static Function * check_container(bean_State * B, TValue * value) {
  switch(value -> tt_) {
    case(BEAN_TSTRING): {
      return G(B) -> String;
    }
    default: {
      printf("Need more code!!");
      return NULL;
    }
  }
}

static Scope * find_variable_scope(bean_State * B, TValue * name) {
  TValue * res;
  Scope * scope = B->l_G->cScope;
  while (scope) {
    res = hash_get(B, scope->variables, name);
    if (res) break;
    scope = scope -> previous;
  }
  return scope;
}

static TValue * find_variable(bean_State * B, TValue * name) {
  TValue * res;
  Scope * scope = B->l_G->cScope;
  while (scope) {
    res = hash_get(B, scope->variables, name);
    if (res) break;
    scope = scope -> previous;
  }
  return res;
}

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
  TokenType op = expression -> infix.op;
  TValue * v = malloc(sizeof(TValue));


#define cal_statement(action) do {                      \
    TValue * v1 = eval(B, expression -> infix.left);    \
    TValue * v2 = eval(B, expression -> infix.right);   \
    bu_byte isfloat = ttisfloat(v1) || ttisfloat(v1);   \
    if (isfloat) {                                      \
      v -> value_.n = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMFLT;                          \
    } else {                                            \
      v -> value_.i = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMINT;                          \
    }                                                   \
  } while(0)

#define compare_statement(action) do {                      \
    TValue * v1 = eval(B, expression -> infix.left);        \
    TValue * v2 = eval(B, expression -> infix.right);       \
    v -> value_.b = action(nvalue(v1), nvalue(v2));         \
    v -> tt_ = BEAN_TBOOLEAN;                               \
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
    case(TK_EQ):
      compare_statement(eq);
      break;
    case(TK_GE):
      compare_statement(gte);
      break;
    case(TK_GT):
      compare_statement(gt);
      break;
    case(TK_LE):
      compare_statement(lte);
      break;
    case(TK_LT):
      compare_statement(lt);
      break;
    case(TK_NE):
      compare_statement(neq);
      break;
    case(TK_ASSIGN): {
      TString * ts = expression -> infix.left->gvar.name;
      TValue * left = malloc(sizeof(TValue));
      setsvalue(left, ts);
      TValue * right = eval(B, expression -> infix.right);
      Scope * scope = find_variable_scope(B, left);
      if (!scope) eval_error("Can't reference the variable before defined");
      hash_set(B, scope->variables, left, right);
      break;
    }
    case(TK_DOT): {
      expr * left = expression -> infix.left;
      TValue * this = eval(B, left);
      expr * right = expression -> infix.right;
      TValue * mname = get_callee_name(B, right);
      Function * fun = check_container(B, this);
      TValue * method = hash_get(B, fun->p->attrs, mname);
      if (!method) eval_error("Can't not call the method which is undefined");
      assert(ttistool(method));
      return tlvalue(method) -> function(B, this, expression -> infix.right);
    }
    default:
      printf("need more code!");
  }

  #undef cal_statement
  #undef compare_statement
  return v;
}

static TValue * function_eval (bean_State * B UNUSED, struct expr * expression) {
  Hash * h = G(B) -> cScope -> variables;
  TValue * func = malloc(sizeof(TValue));
  TValue * name = malloc(sizeof(TValue));
  Function * f = expression -> fun;
  setsvalue(name, f->p->name);
  setfcvalue(func, f);
  hash_set(B, h, name, func);
  return func;
}

static TValue * variable_get_eval (bean_State * B UNUSED, struct expr * expression) {
  TString * variable = expression->gvar.name;
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, variable);
  TValue * value = find_variable(B, name);
  if (!value) eval_error("Can't reference the variable before defined");
  return value;
}

static TValue * variable_define_eval (bean_State * B UNUSED, struct expr * expression) {
  TString * variable = expression->var.name;
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, variable);
  TValue * value = eval(B, expression->var.value);
  SCSV(B, name, value);
  return value;
}

static TValue * loop_eval(bean_State * B, struct expr * expression) {
  expr * condition = expression->loop.condition;
  dynamic_expr * body = expression->loop.body;

  enter_scope(B);
  while (tvalue(eval(B, condition))) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];

      if (ep -> type == EXPR_BREAK) {
        goto breakoff; // Only one jump point, Use goto statement can be more clear and easy
      } else {
        eval(B, ep);
      }
    }
  }

 breakoff:
  leave_scope(B);
  TValue * nilValue = malloc(sizeof(TValue));
  setnilvalue(nilValue);
  return nilValue;
}

static TValue * return_eval(bean_State * B, struct expr * expression) {
  expr * retValue = expression->ret.ret_val;
  return eval(B, retValue);
}

static TValue * function_call_eval (bean_State * B, struct expr * expression) {
  TValue * ret;

  TString * funcName = expression->call.callee;
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, funcName);
  enter_scope(B);
  TValue * func = find_variable(B, name);

  if (checktag(func, BEAN_TTOOL)) {
    Tool * t = tlvalue(func);
    TValue * v = malloc(sizeof(TValue));
    setnilvalue(v);
    ret = t -> function(B, v, expression);
   } else {
    Function * f = fcvalue(func);
    assert(expression->call.args -> count == f->p->arity);
    for (int i = 0; i < expression->call.args -> count; i++) {
      TValue * key = malloc(sizeof(TValue));
      setsvalue(key, f->p->args[i]);
      SCSV(B, key, eval(B, expression->call.args->es[i]));
    }
    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      ret = eval(B, ex);
      if (ex->type == EXPR_RETURN) break;
    }
  }

  leave_scope(B);
  return ret;
}

static TValue * branch_eval(bean_State * B, struct expr * expression) {
  expr * condition = expression->branch.condition;
  dynamic_expr * body;
  enter_scope(B);

  if (tvalue(eval(B, condition))) {
    body = expression->branch.if_body;
  } else {
    body = expression->branch.else_body;
  }

  for (int i = 0; i < body->count; i++) {
    expr * ep = body->es[i];
    eval(B, ep);
  }

  leave_scope(B);
  TValue * nilValue = malloc(sizeof(TValue));
  setnilvalue(nilValue);
  return nilValue;
}

static TValue * string_eval(bean_State * B UNUSED, struct expr * expression) {
  TString * str = expression->sval;
  TValue * value = malloc(sizeof(TValue));
  setsvalue(value, str);
  return value;
}

eval_func fn[] = {
   int_eval,
   float_eval,
   boolean_eval,
   binary_eval,
   string_eval,
   function_eval,
   variable_define_eval,
   variable_get_eval,
   function_call_eval,
   return_eval,
   loop_eval,
   branch_eval
};


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
  scope->variables = init_hash(B);
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

// Add some default tool functions
void add_tools(bean_State * B) {
  Hash * variables = B -> l_G -> globalScope -> variables;
  TValue * name = malloc(sizeof(TValue));
  TString * ts = beanS_newlstr(B, "print", 5);
  setsvalue(name, ts);

  Tool * tool = malloc(sizeof(Tool));
  tool -> function = primitive_print;
  TValue * func = malloc(sizeof(TValue));
  settlvalue(func, tool);
  hash_set(B, variables, name, func);
}

static void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, beanS_newlstr(B, method, len));
  TValue * func = malloc(sizeof(TValue));
  Tool * t = malloc(sizeof(Tool));
  t -> function = fn;
  settlvalue(func, t);
  hash_set(B, h, name, func);
}

Function * init_String(bean_State * B) {
  Proto * p = malloc(sizeof(Proto));
  p -> name = beanS_newlstr(B, "String", 6);
  p -> args = NULL;
  p -> arity = 0;
  p -> attrs = init_hash(B);
  set_prototype_function(B, "equal", 5, primitive_String_equal, p->attrs);
  set_prototype_function(B, "concat", 6, primitive_String_concat, p->attrs);

  Function * f = malloc(sizeof(Function));
  f -> p = p;
  f -> body = NULL;
  f -> static_attrs = init_hash(B);
  return f;
}

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  G -> strt = stringtable_init(B);
  G -> globalScope = create_scope(B, NULL);
  G -> cScope = G -> globalScope;
  B -> l_G = G;
  add_tools(B);
  G -> String = init_String(B);
}

TValue * eval(bean_State * B, struct expr * expression) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression);
}
