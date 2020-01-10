#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "bstring.h"
#include "berror.h"
#include "barray.h"
#include "bobject.h"
#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "mem.h"
#define MIN_STRT_SIZE 64

typedef int (*eval_func) (bean_State * B, struct expr * expression, TValue * ret);
char * rootDir = NULL;

void call_stack_create_frame(bean_State * B, TValue * thisPtr) {
  Mbuffer * stack = G(B)->callStack;
  uint32_t need = 1 + sizeof(TValue *);

   if (stack->buffsize < stack->n + need) {
    if (stack->buffsize + BEAN_MINBUFFER * need >= BUFFER_MAX) {
      runtime_error(B, "%s", "Call stack overflow");
    }

    beanZ_resizebuffer(B, stack, stack->buffsize + BEAN_MINBUFFER * need);
  }
  memcpy(stack->buffer + stack->n, &thisPtr, sizeof(TValue*));
  stack->buffer[stack->n + sizeof(TValue*)] = false;
  stack->n += need;
}

void call_stack_frame_will_recycle(bean_State * B) {
  Mbuffer * stack = G(B)->callStack;
  stack->buffer[stack->n - 1] = true;
}

void call_stack_restore_frame(bean_State * B) {
  Mbuffer * stack = G(B)->callStack;
  uint32_t need = 1 + sizeof(TValue*);

  /* if (stack->buffsize - BEAN_MINBUFFER * need > stack->n) { */
  /*   beanZ_resizebuffer(B, stack, stack->buffsize - BEAN_MINBUFFER * need); */
  /* } */

  if (stack->n >= need) {
    stack->buffer[stack->n-1] = false;
    stack->n -= need;
  } else {
    runtime_error(B, "%s", "Empty of the call stack");
  }
}

char call_stack_peek(bean_State * B) {
  Mbuffer * stack = G(B)->callStack;
  return stack->buffer[stack->n - 1];
}

TValue * call_stack_peek_context(bean_State * B) {
  Mbuffer * stack = G(B)->callStack;
  uint32_t need = 1 + sizeof(TValue*);
  TValue ** tvPtr = malloc(sizeof(TValue*));
  memcpy((char*)tvPtr, stack->buffer + stack->n - need, sizeof(TValue*));

  return *tvPtr;
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

static TValue * search_from_prototype_link(bean_State * B UNUSED, TValue * object, TValue * name) {
  TValue * value = NULL;
  if (ttishash(object)) { // Search in current hash
    Hash * hash = hhvalue(object);
    value = hash_get(B, hash, name);
  }

  if (!value) {
    // Search in current hash
    TValue * proto = object -> prototype;
    while (proto && !value) {
      value = hash_get(B, hhvalue(proto), name);
      proto = proto -> prototype;
    }
    if (!proto) runtime_error(B, "%s", "Can not find the attribute from prototype chain.");
  }

  return value;
}

// Nil have some copy
static int nil_eval (bean_State * B UNUSED, struct expr * expression UNUSED, TValue * ret) {
  setnilvalue(ret);
  return BEAN_OK;
}

static int int_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  ret->tt_ = BEAN_TNUMINT;
  ret->value_.i = expression -> ival;
  return BEAN_OK;
}

static int float_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  ret->tt_ = BEAN_TNUMFLT;
  ret->value_.n = expression -> nval;
  return BEAN_OK;
}

static int boolean_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  ret->tt_ = BEAN_TBOOLEAN;
  ret->value_.b = expression -> bval;
  return BEAN_OK;
}

static int unary_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  TValue * value = malloc(sizeof(TValue));
  eval(B, expression->unary.val, value);

  switch(expression->unary.op) {
    case(TK_SUB):
      if (ttisinteger(value)) {
        setivalue(value, -ivalue(value));
      } else if (ttisfloat(value)) {
        setfltvalue(value, -fltvalue(value));
      }
      break;
    case(TK_ADD):
      break;
    default:
      eval_error(B, "%s", "Unary expression must follow a number");
  }

  *ret = *value;
  return BEAN_OK;
}

static int binary_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  TokenType op = expression -> infix.op;

#define cal_statement(action) do {                                      \
    TValue * v1 = malloc(sizeof(TValue));                               \
    TValue * v2 = malloc(sizeof(TValue));                               \
    eval(B, expression -> infix.left, v1);                              \
    eval(B, expression -> infix.right, v2);                            \
    bu_byte isfloat = ttisfloat(v1) || ttisfloat(v1);   \
    if (isfloat) {                                      \
      ret->value_.n = action(nvalue(v1), nvalue(v2));  \
      ret->tt_ = BEAN_TNUMFLT;                             \
    } else {                                            \
      ret->value_.i = action(nvalue(v1), nvalue(v2));  \
      ret->tt_ = BEAN_TNUMINT;                             \
    }                                                   \
  } while(0)

#define compare_statement(action) do {                      \
    TValue * v1 = malloc(sizeof(TValue));                               \
    TValue * v2 = malloc(sizeof(TValue));                               \
    eval(B, expression -> infix.left, v1);                             \
    eval(B, expression -> infix.right, v2);                \
    ret->value_.b = action(nvalue(v1), nvalue(v2));        \
    ret->tt_ = BEAN_TBOOLEAN;                                  \
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
      TValue * value = malloc(sizeof(TValue));

      switch(leftExpr->type) {
        case(EXPR_GVAR): {
          TValue * left = malloc(sizeof(TValue));
          setsvalue(left, expression -> infix.left->gvar.name);
          eval(B, expression -> infix.right, value);
          Scope * scope = find_variable_scope(B, left);
          if (!scope) eval_error(B, "%s", "Can't reference the variable before defined.");
          hash_set(B, scope->variables, left, value);
          break;
        }
        case(EXPR_BINARY): {
          expr * ep = leftExpr -> infix.left;
          TValue * object = malloc(sizeof(TValue));
          eval(B, ep, object);
          eval(B, expression -> infix.right, value);
          assert(ttishash(object) || ttisarray(object));

          switch(leftExpr -> infix.op) {
            case(TK_DOT): {
              assert(ttishash(object));
              assert(leftExpr -> infix.right->type == EXPR_GVAR);
              TString * ts = leftExpr -> infix.right -> gvar.name;
              TValue * key = malloc(sizeof(TValue));
              setsvalue(key, ts);
              hash_set(B, hhvalue(object), key, value);
              break;
            }
            case(TK_LEFT_BRACKET): {
              assert(ttishash(object) || ttisarray(object));
              if (ttishash(object)) {
                assert(leftExpr -> infix.right->type == EXPR_STRING);
                TString * ts = leftExpr -> infix.right -> sval;
                TValue * key = malloc(sizeof(TValue));
                setsvalue(key, ts);
                hash_set(B, hhvalue(object), key, value);
              } else if (ttisarray(object)) {
                assert(leftExpr -> infix.right->type == EXPR_NUM);
                bean_Integer i = leftExpr -> infix.right -> ival;
                array_set(B, arrvalue(object), i, value);
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

      *ret = *value;
      break;
    }
    case(TK_DOT): {
      TValue * object = malloc(sizeof(TValue));
      TValue * name = malloc(sizeof(TValue));
      eval(B, expression -> infix.left, object);
      expr * right = expression -> infix.right;
      setsvalue(name, right->gvar.name);
      TValue * value = search_from_prototype_link(B, object, name);

      if (ttisfunction(value)) { // setting context for function
        fcvalue(value)->context = object;
      } else if (ttistool(value)) {
        Tool * tl = tlvalue(value);
        tl->context = object;

        if (tl->getter) {
          tl->function(B, object, NULL, 0, &value);
        }
      }
      *ret = *value;
      break;
    }
    case(TK_LEFT_BRACKET): {
      TValue * object = malloc(sizeof(TValue));
      eval(B, expression -> infix.left, object);
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

      *ret = *value;
      break;
    }
    default: {
      printf("need more code!");
      return BEAN_FAIL;
    }

  }

  #undef cal_statement
  #undef compare_statement
  return BEAN_OK;
}

static int self_eval (bean_State * B UNUSED, struct expr * expression UNUSED, TValue * ret) {
  if (G(B)->callStack->n) {
    *ret = *(call_stack_peek_context(B));
  } else {
    *ret = *(G(B) -> nil);
  }
  return BEAN_OK;
}

static int function_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  Function * f = expression -> fun;
  setfcvalue(ret, f);

  if (f->p->name && !f->p->assign) { // Named function and not assigning
    TValue * name = malloc(sizeof(TValue));
    setsvalue(name, f->p->name);
    SCSV(B, name, ret);
  }

  return BEAN_OK;
}

static int variable_get_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  TValue * name = malloc(sizeof(TValue));
  TString * variable = expression->gvar.name;
  setsvalue(name, variable);

  TValue * value = find_variable(B, name);
  if (!value) eval_error(B, "%s", "Can't reference the variable before defined.");
  *ret = *value;

  return BEAN_OK;
}

static int variable_define_eval (bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  TValue * name = malloc(sizeof(TValue));
  TString * vname = expression->var.name;
  setsvalue(name, vname);

  TValue * value = malloc(sizeof(TValue));
  eval(B, expression->var.value, value);
  if (ttisfunction(value)) {
    TString * funName = fcvalue(value)->p->name;
    if (!funName) { // anonymous function
      fcvalue(value)->p->name = vname;
    }
  }

  SCSV(B, name, value);
  *ret = *value;
  return BEAN_OK;
}

static int loop_eval(bean_State * B, struct expr * expression, TValue *ret) {
  #define condition_is_true eval(B, expression->loop.condition, cond) && truthvalue(cond)

  dynamic_expr * body = expression->loop.body;
  TValue * cond = malloc(sizeof(TValue));
  TValue * stmt = malloc(sizeof(TValue));
  enter_scope(B);

  while (condition_is_true) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];

      if (ep -> type == EXPR_BREAK || call_stack_peek(B)) {
        goto breakoff; // Only one jump point, Use goto statement can be more clear and easy
      } else {
        eval(B, ep, stmt);
      }
    }
  }

 breakoff:
  leave_scope(B);
  *ret = *stmt;

  #undef condition_is_true
  return BEAN_OK;
}

static int return_eval(bean_State * B, struct expr * expression, TValue * ret) {
  call_stack_frame_will_recycle(B);
  eval(B, expression->ret.ret_val, ret);
  return BEAN_OK;
}

static int function_call_eval (bean_State * B, struct expr * expression, TValue * ret) {
  TValue * func = malloc(sizeof(TValue));
  eval(B, expression->call.callee, func);
  enter_scope(B);

  if (ttistool(func)) {
    Tool * t = tlvalue(func);
    // Use binding context

    TValue * this = malloc(sizeof(TValue));

    if (expression->call.callee->type == EXPR_BINARY) {
      eval(B, expression->call.callee->infix.left, this);
    } else {
      this = G(B)->nil;
    }

    TValue * args = malloc(sizeof(TValue) * 16); // TODO: 调用栈，参数
    int i;

    for (i = 0; i < expression->call.args->count; i++) {
      TValue * val = malloc(sizeof(TValue));
      eval(B, expression->call.args->es[i], val);
      args[i] = *val;
    }

    t->function(B, this, args, i, &ret);
  } else if (ttisfunction(func)) {
    Function * f = fcvalue(func);
    call_stack_create_frame(B, f->context);

    for (int i = 0; i < f->p->arity; i++) {
      TValue * key = malloc(sizeof(TValue));
      setsvalue(key, f->p->args[i]);

      expr * param = expression->call.args->es[i];

      if (param) {
        TValue * value = malloc(sizeof(TValue));
        eval(B, expression->call.args->es[i], value);
        SCSV(B, key, value);
      } else {
        SCSV(B, key, G(B)->nil);
      }
    }

    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      eval(B, ex, ret);
      if (call_stack_peek(B)) break;
    }
    call_stack_restore_frame(B);
  } else {
    eval_error(B, "%s", "You are trying to call which is not a function.");
  }

  leave_scope(B);

  return BEAN_OK;
}

static int branch_eval(bean_State * B, struct expr * expression, TValue * ret) {
  expr * condition = expression->branch.condition;
  dynamic_expr * body;
  enter_scope(B);
  TValue * cond = malloc(sizeof(TValue));
  eval(B, condition, cond);

  if (truthvalue(cond)) {
    body = expression->branch.if_body;
  } else {
    body = expression->branch.else_body;
  }

  TValue * retVal = malloc(sizeof(TValue));

  if (body) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];
      eval(B, ep, retVal);

      if (call_stack_peek(B)) break;
    }
  } else {
    retVal = G(B) -> nil;
  }

  *ret = *retVal;
  leave_scope(B);
  return BEAN_OK;
}

static int string_eval(bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  TString * str = expression->sval;
  setsvalue(ret, str);
  return BEAN_OK;
}

static int array_eval(bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  dynamic_expr * eps = expression->array;
  Array * array = init_array(B);

  for (int i = 0; i < eps->count; i++) {
    TValue * val = malloc(sizeof(TValue));
    eval(B, eps->es[i], val);
    array_push(B, array, val);
  }
  setarrvalue(ret, array);
  return BEAN_OK;
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
      eval_error(B, "%s", "Invalid key.");
      return G(B)->nil;
    }
  }
}

static int hash_eval(bean_State * B UNUSED, struct expr * expression, TValue * ret) {
  dynamic_expr * eps = expression->hash;
  Hash * hash = init_hash(B);
  assert(eps->count % 2 == 0);
  for (int i = 0; i < eps->count; i+=2) {
    TValue * key = hash_key_eval(B, eps->es[i]);
    TValue * value = malloc(sizeof(TValue));
    eval(B, eps->es[i + 1], value);

    if (ttisfunction(value) && !fcvalue(value)->p->name) {
      fcvalue(value)->p->name = svalue(key);
    }

    hash_set(B, hash, key, value);
  }
  sethashvalue(ret, hash);
  return BEAN_OK;
}

eval_func fn[] = {
   nil_eval,
   int_eval,
   float_eval,
   boolean_eval,
   unary_eval,
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

static char * read_source_file(bean_State * B, const char * path) {
  FILE * file = fopen(path, "r");

    if (file == NULL) io_error(B, "%s", "Can not open file.");

  struct stat fileStat;
  stat(path, &fileStat);
  size_t fileSize = fileStat.st_size;

  printf("File size is: %lu.\n", fileSize);
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
  printf("Source file name is %s.\n", getstr(e));
  char* source = read_source_file(B, path);
  beanX_setinput(B, source, e, *source);

  bparser(B->ls);
}

void run() {
  char * source;
  size_t buffersize = 200;
  size_t len;
  source = (char *)malloc(buffersize * sizeof(char));

  if( source == NULL) {
    perror("Unable to allocate buffer");
    exit(1);
  }

  bean_State * B = bean_State_init();
  TString * e = beanS_newlstr(B, "REPL", 4);

  while(true) {
    printf("> ");
    len = getline(&source, &buffersize, stdin);
    if (len == 1 && source[0] == '\n') continue;
    beanX_setinput(B, source, e, *source);
    TValue * value = malloc(sizeof(TValue));
    bparser_for_line(B->ls, value);
    TValue * string = tvalue_inspect_pure(B, value);
    TString * ts = svalue(string);
    printf("=> %s\n", getstr(ts));
  }
}

static Tool * initialize_tool_by_fn(primitive_Fn fn, bool getter) {
  Tool * t = malloc(sizeof(Tool));
  t -> function = fn;
  t -> context = NULL;
  t -> getter = getter;
  return t;
}

// Add some default tool functions
static void add_tools(bean_State * B) {
  Hash * variables = B -> l_G -> globalScope -> variables;
  TValue * name = malloc(sizeof(TValue));
  TString * ts = beanS_newlstr(B, "print", 5);
  setsvalue(name, ts);

  TValue * func = malloc(sizeof(TValue));
  Tool * tool = initialize_tool_by_fn(primitive_print, false);

  settlvalue(func, tool);
  hash_set(B, variables, name, func);
}

static void set_prototype(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h, bool getter) {
  TValue * name = malloc(sizeof(TValue));
  setsvalue(name, beanS_newlstr(B, method, len));
  TValue * func = malloc(sizeof(TValue));
  Tool * t = initialize_tool_by_fn(fn, getter);
  settlvalue(func, t);
  hash_set(B, h, name, func);
}

void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  set_prototype(B, method, len, fn, h, false);
}

void set_prototype_getter(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h) {
  set_prototype(B, method, len, fn, h, true);
}

TValue * init_Nil(bean_State * B UNUSED) {
  TValue * proto = malloc(sizeof(TValue));
  setnilvalue(proto);
  proto -> prototype = NULL;
  return proto;
}

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  G -> strt = stringtable_init(B);
  G -> callStack = malloc(sizeof(Mbuffer));
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

  beanZ_initbuffer(G->callStack);
}

int eval(bean_State * B, struct expr * expression, TValue * ret) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression, ret);
}
