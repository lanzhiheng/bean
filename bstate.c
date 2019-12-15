#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "bstring.h"
#include "bobject.h"
#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "mem.h"
#define MIN_STRT_SIZE 64

typedef TValue* (*eval_func) (bean_State * B, struct expr * expression, TValue * context);
char * rootDir = NULL;

#define io_error(message) printf("%s", message)
#define mem_error(message) printf("%s", message)
#define eval_error(message) ({                  \
      printf("%s\n", message);                    \
      abort();                                  \
})

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

static TValue * search_from_prototype_link(bean_State * B UNUSED, TValue * object, TValue * name) {
  TValue * value = NULL;
  if (ttishash(object)) { // Search in current hash
    Hash * hash = hhvalue(object);
    value = hash_get(B, hash, name);
  }

  // Search in current hash
  TValue * proto = object -> prototype;
  while (proto && !value) {
    value = hash_get(B, hhvalue(proto), name);
    proto = proto -> prototype;
  }
  return value ? value : G(B)->nil;
}

static TValue * nil_eval (bean_State * B UNUSED, struct expr * expression UNUSED, TValue * context UNUSED) {
  return G(B)->nil;
}

static TValue * int_eval (bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMINT;
  v -> value_.i = expression -> ival;
  return v;
}

static TValue * float_eval (bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMFLT;
  v -> value_.n = expression -> nval;
  return v;
}

static TValue * boolean_eval (bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TBOOLEAN;
  v -> value_.b = expression -> bval;
  return v;
}

static TValue * binary_eval (bean_State * B UNUSED, struct expr * expression, TValue * context) {
  TokenType op = expression -> infix.op;
  TValue * v = malloc(sizeof(TValue));

#define cal_statement(action) do {                                      \
    TValue * v1 = eval(B, expression -> infix.left, context);           \
    TValue * v2 = eval(B, expression -> infix.right, context);          \
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
    TValue * v1 = eval(B, expression -> infix.left, context);       \
    TValue * v2 = eval(B, expression -> infix.right, context);              \
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
      expr * leftExpr = expression -> infix.left;
      switch(leftExpr->type) {
        case(EXPR_GVAR): {
          TValue * left = malloc(sizeof(TValue));
          setsvalue(left, expression -> infix.left->gvar.name);
          TValue * right = eval(B, expression -> infix.right, context);
          Scope * scope = find_variable_scope(B, left);
          if (!scope) eval_error("Can't reference the variable before defined");
          hash_set(B, scope->variables, left, right);
        }
        case(EXPR_BINARY): {
          expr * ep = leftExpr -> infix.left;
          TValue * object = eval(B, ep, context);
          TValue * source = eval(B, expression -> infix.right, context);
          assert(ttishash(object) || ttisarray(object));

          switch(leftExpr -> infix.op) {
            case(TK_DOT): {
              assert(ttishash(object));
              assert(leftExpr -> infix.right->type == EXPR_GVAR);
              TString * ts = leftExpr -> infix.right -> gvar.name;
              TValue * key = malloc(sizeof(TValue));
              setsvalue(key, ts);
              hash_set(B, hhvalue(object), key, source);
              break;
            }
            case(TK_LEFT_BRACKET): {
              assert(ttishash(object) || ttisarray(object));
              if (ttishash(object)) {
                assert(leftExpr -> infix.right->type == EXPR_STRING);
                TString * ts = leftExpr -> infix.right -> sval;
                TValue * key = malloc(sizeof(TValue));
                setsvalue(key, ts);
                hash_set(B, hhvalue(object), key, source);
              } else if (ttisarray(object)) {
                assert(leftExpr -> infix.right->type == EXPR_NUM);
                bean_Integer i = leftExpr -> infix.right -> ival;
                array_set(B, arrvalue(object), i, source);
              }
              break;
            }
          }

          break;
        }
        default: {
          printf("Not an assignable value");
        }
      }
      break;
    }
    case(TK_DOT): {
      TValue * object = eval(B, expression -> infix.left, context);
      expr * right = expression -> infix.right;
      TValue * name = malloc(sizeof(TValue));
      setsvalue(name, right->gvar.name);
      TValue * value = search_from_prototype_link(B, object, name);

      if (ttisfunction(value)) { // setting context for function
        fcvalue(value)->context = object;
      } else if (ttistool(value)) {
        tlvalue(value)->context = object;
      }
      return value;
    }
    case(TK_LEFT_BRACKET): {
      TValue * object = eval(B, expression -> infix.left, context);
      expr * right = expression -> infix.right;
      TValue * value = NULL;

      if (ttisarray(object) && right->type == EXPR_NUM) { // Array search by index
        bean_Integer i = right -> ival;
        value = array_get(B, arrvalue(object), i);
      } else if (right->type == EXPR_STRING) { // Attributes search
        TString * ts = right -> sval;
        TValue * name = malloc(sizeof(TValue));
        setsvalue(name, ts);
        value = search_from_prototype_link(B, object, name);
      }

      return value;
    }
    default:
      printf("need more code!");
  }

  #undef cal_statement
  #undef compare_statement
  return v;
}

static TValue * self_eval (bean_State * B UNUSED, struct expr * expression UNUSED, TValue * context) {
  return context;
}

static TValue * function_eval (bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TValue * func = malloc(sizeof(TValue));
  Function * f = expression -> fun;
  setfcvalue(func, f);

  if (f->p->name && !f->p->assign) { // Named function and not assigning
    TValue * name = malloc(sizeof(TValue));
    setsvalue(name, f->p->name);
    SCSV(B, name, func);
  }

  return func;
}

static TValue * variable_get_eval (bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TString * variable = expression->gvar.name;
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, variable);
  TValue * value = find_variable(B, name);
  if (!value) eval_error("Can't reference the variable before defined");
  return value;
}

static TValue * variable_define_eval (bean_State * B UNUSED, struct expr * expression, TValue * context) {
  TString * vname = expression->var.name;
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, vname);

  TValue * value = eval(B, expression->var.value, context);
  if (ttisfunction(value)) {
    TString * funName = fcvalue(value)->p->name;
    if (!funName) { // anonymous function
      fcvalue(value)->p->name = vname;
    }
  }

  SCSV(B, name, value);
  return value;
}

static TValue * loop_eval(bean_State * B, struct expr * expression, TValue * context) {
  expr * condition = expression->loop.condition;
  dynamic_expr * body = expression->loop.body;

  enter_scope(B);
  while (tvalue(eval(B, condition, context))) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];

      if (ep -> type == EXPR_BREAK) {
        goto breakoff; // Only one jump point, Use goto statement can be more clear and easy
      } else {
        eval(B, ep, context);
      }
    }
  }

 breakoff:
  leave_scope(B);
  return G(B)->nil;
}

static TValue * return_eval(bean_State * B, struct expr * expression, TValue * context) {
  expr * retValue = expression->ret.ret_val;
  return eval(B, retValue, context);
}

static TValue * function_call_eval (bean_State * B, struct expr * expression, TValue * context) {
  TValue * ret = NULL;
  enter_scope(B);
  TValue * func = eval(B, expression->call.callee, context);

  if (checktag(func, BEAN_TTOOL)) {
    Tool * t = tlvalue(func);
    // Use binding context
    TValue * selfContext = t->context ? t->context : context;
    TValue * v = NULL;
    if (expression->call.callee->type == EXPR_BINARY) {
      v = eval(B, expression->call.callee->infix.left, context);
    } else {
      v = G(B)->nil;
    }

    ret = t -> function(B, v, expression, selfContext);
  } else if (checktag(func, BEAN_TFUNCTION)) {
    Function * f = fcvalue(func);

    // Use binding context
    TValue * selfContext = f->context ? f->context : context;
    assert(expression->call.args -> count == f->p->arity);
    for (int i = 0; i < expression->call.args -> count; i++) {
      TValue * key = malloc(sizeof(TValue));
      setsvalue(key, f->p->args[i]);
      SCSV(B, key, eval(B, expression->call.args->es[i], context));
    }
    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      ret = eval(B, ex, selfContext);
      if (ex->type == EXPR_RETURN) break;
    }
  } else {
    eval_error("You are trying to call which is not a function.");
  }

  leave_scope(B);
  return ret;
}

static TValue * branch_eval(bean_State * B, struct expr * expression, TValue * context) {
  expr * condition = expression->branch.condition;
  dynamic_expr * body;
  enter_scope(B);

  if (tvalue(eval(B, condition, context))) {
    body = expression->branch.if_body;
  } else {
    body = expression->branch.else_body;
  }

  for (int i = 0; i < body->count; i++) {
    expr * ep = body->es[i];
    eval(B, ep, context);
  }

  leave_scope(B);
  return G(B)->nil;
}

static TValue * string_eval(bean_State * B UNUSED, struct expr * expression, TValue * context UNUSED) {
  TString * str = expression->sval;
  TValue * value = malloc(sizeof(TValue));
  setsvalue(value, str);
  return value;
}

static TValue * array_eval(bean_State * B UNUSED, struct expr * expression, TValue * context) {
  dynamic_expr * eps = expression->array;
  Array * array = init_array(B);

  for (int i = 0; i < eps->count; i++) {
    array_push(B, array, eval(B, eps->es[i], context));
  }
  TValue * value = malloc(sizeof(TValue));
  setarrvalue(value, array);
  return value;
}

static TValue * hash_key_eval(bean_State * B UNUSED, struct expr * expression) {
  switch(expression->type) {
    case(EXPR_GVAR): {
      TString * name = expression->gvar.name;
      TValue * n = malloc(sizeof(TValue));
      setsvalue(n, name);
      return n;
    }
    default: {
      eval_error("Invalid key");
    }
  }
}

static TValue * hash_eval(bean_State * B UNUSED, struct expr * expression, TValue * context) {
  dynamic_expr * eps = expression->hash;
  Hash * hash = init_hash(B);
  assert(eps->count % 2 == 0);
  for (int i = 0; i < eps->count; i+=2) {
    TValue * key = hash_key_eval(B, eps->es[i]);
    TValue * value = eval(B, eps->es[i + 1], context);
    if (ttisfunction(value) && !fcvalue(value)->p->name) {
      fcvalue(value)->p->name = svalue(key);
    }

    hash_set(B, hash, key, value);
  }
  TValue * value = malloc(sizeof(TValue));
  sethashvalue(value, hash);
  return value;
}

eval_func fn[] = {
   nil_eval,
   int_eval,
   float_eval,
   boolean_eval,
   binary_eval,
   string_eval,
   function_eval,
   self_eval,
   variable_define_eval,
   variable_get_eval,
   function_call_eval,
   return_eval,
   loop_eval,
   branch_eval,
   array_eval,
   hash_eval
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

static Tool * initialize_tool_by_fn(primitive_Fn fn) {
  Tool * t = malloc(sizeof(Tool));
  t -> function = fn;
  t -> context = NULL;
  return t;
}

// Add some default tool functions
static void add_tools(bean_State * B) {
  Hash * variables = B -> l_G -> globalScope -> variables;
  TValue * name = malloc(sizeof(TValue));
  TString * ts = beanS_newlstr(B, "print", 5);
  setsvalue(name, ts);

  TValue * func = malloc(sizeof(TValue));
  Tool * tool = initialize_tool_by_fn(primitive_print);

  settlvalue(func, tool);
  hash_set(B, variables, name, func);
}

static void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, beanS_newlstr(B, method, len));
  TValue * func = malloc(sizeof(TValue));
  Tool * t = initialize_tool_by_fn(fn);
  settlvalue(func, t);
  hash_set(B, h, name, func);
}

TValue * init_String(bean_State * B) {
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "equal", 5, primitive_String_equal, hhvalue(proto));
  set_prototype_function(B, "concat", 6, primitive_String_concat, hhvalue(proto));
  return proto;
}

TValue * init_Array(bean_State * B) {
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  return proto;
}

TValue * init_Nil(bean_State * B UNUSED) {
  TValue * proto = malloc(sizeof(TValue));
  setnilvalue(proto);
  return proto;
}

TValue * init_Hash(bean_State * B) {
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);

  sethashvalue(proto, h);
  set_prototype_function(B, "id", 2, primitive_Hash_id, hhvalue(proto));
  return proto;
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
  G -> nil = init_Nil(B);
  G -> sproto = init_String(B);
  G -> aproto = init_Array(B);
  G -> hproto = init_Hash(B);

  G -> sproto -> prototype = G -> hproto;
  G -> aproto -> prototype = G -> hproto;
  G -> hproto -> prototype = G -> nil;
}

TValue * eval(bean_State * B, struct expr * expression, TValue * context) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression, context);
}
