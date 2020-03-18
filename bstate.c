#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "bmeta.h"
#include "bstring.h"
#include "bnumber.h"
#include "bfunction.h"
#include "bmath.h"
#include "bdate.h"
#include "berror.h"
#include "barray.h"
#include "bregex.h"
#include "bobject.h"
#include "bstate.h"
#include "bparser.h"
#include "bhttp.h"
#include "blex.h"
#include "mem.h"

#include <setjmp.h>
jmp_buf buf;
int REPL = false;

#define MIN_STRT_SIZE 64

typedef TValue * (*eval_func) (bean_State * B, struct expr * expression);
char * rootDir = NULL;

void new_loop(bean_State * B) {
  loopStack * stack = G(B)->loopStack;
  loopStack * stackItem = malloc(sizeof(loopStack));
  stackItem -> target = false;
  stackItem -> next = stack;
  G(B) -> loopStack = stackItem;
}

void will_out_loop(bean_State * B) {
  loopStack * stack = G(B)->loopStack;
  if (stack == NULL) {
    runtime_error(B, "%s", "Can not break outside the loop");
  }
  stack -> target = true;
}

void restore_loop(bean_State * B) {
  if (G(B)->loopStack == NULL) {
    runtime_error(B, "%s", "Empty of the call stack");
  } else {
    loopStack * item = G(B)->loopStack;
    G(B)->loopStack = G(B)->loopStack -> next;
    free(item);
  }
}

void call_stack_create_frame(bean_State * B) {
  callStack * stack = G(B)->callStack;
  callStack * stackItem = malloc(sizeof(callStack));

  stackItem -> retVal = NULL;
  stackItem -> target = false;
  stackItem -> next = stack;
  G(B)->callStack = stackItem;
}

char loop_stack_peek(bean_State * B) {
  loopStack * stack = G(B)->loopStack;
  return stack -> target;
}

// Set return target and retValue
void call_stack_frame_will_recycle(bean_State * B, TValue * retVal) {
  callStack * stack = G(B)->callStack;
  if (stack == NULL) {
    runtime_error(B, "%s", "Can not return outside the function");
  }
  stack -> target = true;
  stack -> retVal = retVal;
}

void call_stack_restore_frame(bean_State * B) {
  if (G(B)->callStack == NULL) {
    runtime_error(B, "%s", "Empty of the call stack");
  } else {
    callStack * item = G(B)->callStack;
    G(B)->callStack = G(B)->callStack -> next;
    free(item);
  }
}

TValue * call_stack_peek_return(bean_State * B) {
  callStack * stack = G(B)->callStack;
  return stack -> retVal;
}

char call_stack_peek(bean_State * B) {
  callStack * stack = G(B)->callStack;
  return stack != NULL && stack -> target;
}

void set_self_before_caling(bean_State * B, TValue * context) {
  TValue * key = TV_MALLOC;
  setsvalue(key, beanS_newlstr(B, "self", 4));
  SCSV(B, key, context ? context : NULL);
}

static Scope * find_variable_scope(bean_State * B, TValue * name) {
  TValue * res;
  Scope * scope = G(B) -> cScope;
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
  if (ttisnil(object)) runtime_error(B, "%s", "Can not find the attribute from prototype chain.");

  TValue * value = NULL;
  if (ttishash(object)) { // Search in current hash
    Hash * hash = hhvalue(object);
    value = hash_get(B, hash, name);
  }
  if (value) return value;

  // Search in proto chain
  TValue * proto = object -> prototype;

  while(!ttisnil(proto)) {
    value = hash_get(B, hhvalue(proto), name);
    if (value) return value;
    proto = proto -> prototype;
  }

  runtime_error(B, "%s", "Can not find the attribute from prototype chain.");
  return value;
}

static TValue * nil_eval (bean_State * B UNUSED, struct expr * expression UNUSED) {
  return G(B)->nil;
}

static TValue * number_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * ret = TV_MALLOC;
  setnvalue(ret, expression -> nval);
  return ret;
}

static TValue * boolean_eval (bean_State * B UNUSED, struct expr * expression) {
  return expression -> bval ? G(B)->tVal : G(B)->fVal;
}

static TValue * unary_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * value = eval(B, expression->unary.val);

  switch(expression->unary.op) {
    case(TK_SUB):
      setnvalue(value, -nvalue(value));
      break;
    case(TK_ADD):
      break;
    case(TK_BANG):
    case(TK_NOT):{
      value = truthvalue(value) ?  G(B)->fVal : G(B)->tVal;
      break;
    }
    case(TK_TYPE_OF): {
      value = type_statement(B, value);
      break;
    }
    default:
      eval_error(B, "%s", "Unary expression must follow a number");
  }

  return value;
}

static TValue * change_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * value = eval(B, expression->change.val);
  TValue * retVal = TV_MALLOC;

  switch(expression->change.op) {
    case(TK_ADD): {
      double res = expression->change.prefix ? nvalue(value) + 1 : nvalue(value);
      setnvalue(retVal, res);
      setnvalue(value, nvalue(value) + 1);
      break;
    }
    case(TK_SUB): {
      double res = expression->change.prefix ? nvalue(value) - 1: nvalue(value);
      setnvalue(retVal, res) ;
      setnvalue(value, nvalue(value) - 1);
      break;
    }
    default:
      eval_error(B, "%s", "Just supporting ++ or -- operator.");
  }

  return retVal;
}

static int compareTwoTValue(bean_State * B, TValue * v1, TValue *v2) {
  TValue * convertV1 = ttisstring(v1) ? v1 : tvalue_inspect(B, v1);
  TValue * convertV2 = ttisstring(v2) ? v2 : tvalue_inspect(B, v2);

  return strcmp(getstr(svalue(convertV1)), getstr(svalue(convertV2)));
}

static TValue * binary_eval (bean_State * B, struct expr * expression) {
  TokenType op = expression -> infix.op;
  TValue * ret = TV_MALLOC;

#define cal_statement(action) do {                                      \
    TValue * v1 = eval(B, expression -> infix.left);                    \
    TValue * v2 = eval(B, expression -> infix.right);                   \
    if (!ttisnumber(v1)) {                                              \
      eval_error(B, "%s", "left operand of "#action" must be number");  \
    }                                                                   \
    if (!ttisnumber(v2)) {                                              \
      eval_error(B, "%s", "right operand of "#action" must be number"); \
    }                                                                   \
    if (expression -> infix.assign) ret = v1;                           \
    setnvalue(ret, action(nvalue(v1), nvalue(v2)));                     \
  } while(0)

#define compare_statement(action) do {                                  \
    TValue * v1 = eval(B, expression -> infix.left);                    \
    TValue * v2 = eval(B, expression -> infix.right);                   \
    if (ttisnumber(v1) && ttisnumber(v2)) {                             \
      ret = action(nvalue(v1), nvalue(v2)) ? G(B)->tVal : G(B)->fVal;   \
    } else {                                                            \
      int result = compareTwoTValue(B, v1, v2);                         \
      ret = action(result, 0) ? G(B)->tVal : G(B)->fVal;                \
    }                                                                   \
  } while(0)

  switch(op) {
    case(TK_ADD): {
      TValue * v1 = eval(B, expression -> infix.left);
      TValue * v2 = eval(B, expression -> infix.right);

      if (expression -> infix.assign) ret = v1;

      if (ttisnumber(v1) && ttisnumber(v2)) {
        setnvalue(ret, add(nvalue(v1), nvalue(v2)));
      } else {
        TValue * temp = concat(B, tvalue_inspect(B, v1), tvalue_inspect(B, v2));
        setsvalue(ret, svalue(temp));
      }
      break;
    }
    case(TK_SUB):
      cal_statement(sub);
      break;
    case(TK_MUL):
      cal_statement(mul);
      break;
    case(TK_SHR):
      cal_statement(shr);
      break;
    case(TK_SHL):
      cal_statement(shl);
      break;
    case(TK_DIV):
      cal_statement(div);
      break;
    case(TK_MOD):
      cal_statement(mod);
      break;
    case(TK_LOGIC_AND):
      cal_statement(logic_and);
      break;
    case(TK_LOGIC_OR):
      cal_statement(logic_or);
      break;
    case(TK_LOGIC_XOR):
      cal_statement(logic_xor);
      break;
    case(TK_EQ): {
      TValue * v1 = eval(B, expression -> infix.left);
      TValue * v2 = eval(B, expression -> infix.right);
      ret = check_equal(v1, v2) ? G(B)->tVal : G(B)->fVal;
      break;
    }
    case(TK_GE):
      compare_statement(gte);
      break;
    case(TK_GT):
      compare_statement(gt);
      break;
    case(TK_LE):
      compare_statement(lte);
      break;
    case(TK_AND): {
      ret = eval(B, expression -> infix.left);
      if (truthvalue(ret)) ret = eval(B, expression -> infix.right);
      break;
    }
    case(TK_LT):
      compare_statement(lt);
      break;
    case(TK_OR): {
      ret = eval(B, expression -> infix.left);
      if (falsyvalue(ret)) ret = eval(B, expression -> infix.right);
      break;
    }
    case(TK_NE): {
      TValue * v1 = eval(B, expression -> infix.left);
      TValue * v2 = eval(B, expression -> infix.right);
      ret = !check_equal(v1, v2) ? G(B)->tVal : G(B)->fVal;
      break;
    }
    case(TK_ASSIGN): {
      expr * leftExpr = expression -> infix.left;
      TValue * value = TV_MALLOC;

      switch(leftExpr->type) {
        case(EXPR_GVAR): {
          TValue * left = TV_MALLOC;
          setsvalue(left, expression -> infix.left->var.name);
          value = eval(B, expression -> infix.right);
          Scope * scope = find_variable_scope(B, left);
          if (!scope) eval_error(B, "%s", "Can't reference the variable before defined.");
          hash_set(B, scope->variables, left, value);
          break;
        }
        case(EXPR_BINARY): {
          expr * ep = leftExpr -> infix.left;
          TValue * object = eval(B, ep);
          value = eval(B, expression -> infix.right);
          assert(ttishash(object) || ttisarray(object));

          switch(leftExpr -> infix.op) {
            case(TK_DOT): {
              assert(ttishash(object));
              assert(leftExpr -> infix.right->type == EXPR_GVAR);
              TString * ts = leftExpr -> infix.right -> var.name;
              TValue * key = TV_MALLOC;
              setsvalue(key, ts);
              hash_set(B, hhvalue(object), key, value);
              break;
            }
            case(TK_LEFT_BRACKET): {
              assert(ttishash(object) || ttisarray(object));
              EXPR_TYPE type = leftExpr -> infix.right->type;
              if (ttishash(object)) {
                assert(type == EXPR_STRING || type == EXPR_GVAR);
                TValue * key = eval(B, leftExpr -> infix.right);
                hash_set(B, hhvalue(object), key, value);
              } else if (ttisarray(object)) {
                assert(type == EXPR_NUM || type == EXPR_GVAR);
                TValue * index = eval(B, leftExpr -> infix.right);
                array_set(B, arrvalue(object), nvalue(index), value);
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

      return value;
    }
    case(TK_DOT): {
      TValue * object = eval(B, expression -> infix.left);
      TValue * name = TV_MALLOC;
      expr * right = expression -> infix.right;
      setsvalue(name, right->var.name);
      TValue * value = search_from_prototype_link(B, object, name);

      if (ttisfunction(value)) { // setting context for function
        fcvalue(value)->context = object;
      } else if (ttistool(value)) {
        Tool * tl = tlvalue(value);
        tl->context = object;

        if (tl->getter) {
          value = tl->function(B, object, NULL, 0);
        }
      }
      return value;
    }
    case(TK_LEFT_BRACKET): {
      TValue * object = eval(B, expression -> infix.left);
      expr * right = expression -> infix.right;
      TValue * value = NULL;

      EXPR_TYPE type = right->type;

      if (ttisarray(object)) {
        assert(type == EXPR_NUM || type == EXPR_GVAR);
        TValue * index = eval(B, expression -> infix.right);
        value = array_get(B, arrvalue(object), nvalue(index));
      } else { // Attribute of the string, hash, number, etc
        assert(type == EXPR_STRING || type == EXPR_GVAR);
        TValue * key = eval(B, expression -> infix.right);
        value = search_from_prototype_link(B, object, key);
      }

      return value;
    }
    default: {
      eval_error(B, "%s", "need more code!");
    }

  }

  #undef cal_statement
  #undef compare_statement
  return ret;
}

static TValue * function_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * ret = TV_MALLOC;
  Function * f = expression -> fun;
  setfcvalue(ret, f);

  if (f->p->name && !f->p->assign) { // Named function and not assigning
    TValue * name = TV_MALLOC;
    setsvalue(name, f->p->name);
    SCSV(B, name, ret);
  }

  return ret;
}

static TValue * self_eval (bean_State * B UNUSED, struct expr * expression UNUSED) {
  TValue * name = TV_MALLOC;
  setsvalue(name, beanS_newlstr(B, "self", 4));

  TValue * value = find_variable(B, name);
  if (!value) eval_error(B, "%s", "Can't reference the variable before defined.");

  return value;
}

static TValue * variable_get_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * name = TV_MALLOC;
  TString * variable = expression->var.name;
  setsvalue(name, variable);

  TValue * value = find_variable(B, name);

  if (!value) eval_error(B, "%s", "Can't reference the variable before defined.");

  return value;
}

static TValue * variable_define_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * name = TV_MALLOC;
  TString * vname = expression->var.name;
  setsvalue(name, vname);

  TValue * value = eval(B, expression->var.value);

  if (ttisfunction(value)) {
    TString * funName = fcvalue(value)->p->name;
    if (!funName) { // anonymous function
      fcvalue(value)->p->name = vname;
    }
  }

  SCSV(B, name, value);

  return value;
}

static TValue * loop_eval(bean_State * B, struct expr * expression) {
#define condition_is_true (cond = eval(B, expression->loop.condition)) && truthvalue(cond)

  dynamic_expr * body = expression->loop.body;
  TValue * cond = NULL;
  TValue * stmt = G(B)->nil;
  enter_scope(B);
  new_loop(B);

  if (!expression->loop.firstcheck) { // do-while loop
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];

      if (ep -> type == EXPR_BREAK || call_stack_peek(B)) {
        goto breakoff; // Only one jump point, Use goto statement can be more clear and easy
      } else {
        stmt = eval(B, ep);
      }
    }
  }

  while (condition_is_true) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];

      if (loop_stack_peek(B) || call_stack_peek(B)) {
        goto breakoff; // Only one jump point, Use goto statement can be more clear and easy
      } else {
        stmt = eval(B, ep);
      }
    }
  }

#undef condition_is_true

 breakoff:
  restore_loop(B);
  leave_scope(B);

  return G(B)->nil;
}

static TValue * return_eval(bean_State * B, struct expr * expression) {
  TValue * retVal = eval(B, expression->ret.ret_val);
  call_stack_frame_will_recycle(B, retVal);
  return retVal;
}

static TValue * break_eval(bean_State * B, struct expr * expression) {
  will_out_loop(B);
  return G(B)->nil;
}

static TValue * function_call_eval (bean_State * B, struct expr * expression) {
  TValue * func = eval(B, expression->call.callee);
  TValue * ret = NULL;

  enter_scope(B);

  if (ttistool(func)) {
    Tool * t = tlvalue(func);
    // Use binding context

    TValue * this = NULL;

    if (expression->call.callee->type == EXPR_BINARY) {
      this = eval(B, expression->call.callee->infix.left);
    } else {
      this = G(B)->nil;
    }

    TValue * args = malloc(sizeof(TValue) * 16); // TODO: 调用栈，参数
    int i;

    for (i = 0; i < expression->call.args->count; i++) {
      TValue * val = eval(B, expression->call.args->es[i]);
      args[i] = *val;
    }

    ret = t->function(B, this, args, i);
  } else if (ttisfunction(func)) {
    Function * f = fcvalue(func);
    call_stack_create_frame(B);

    for (int i = 0; i < f->p->arity; i++) {
      TValue * key = TV_MALLOC;
      setsvalue(key, f->p->args[i]);

      expr * param = expression->call.args->es[i];

      if (param) {
        TValue * value = eval(B, expression->call.args->es[i]);
        SCSV(B, key, value);
      } else {
        SCSV(B, key, G(B)->nil);
      }
    }

    set_self_before_caling(B, f->context);

    for (int j = 0; j < f->body->count; j++) {
      expr * ex = f->body->es[j];
      ret = eval(B, ex);

      if (call_stack_peek(B)) {
        ret = call_stack_peek_return(B);
        break;
      }
    }
    call_stack_restore_frame(B);
  } else {
    eval_error(B, "%s", "You are trying to call which is not a function.");
  }

  leave_scope(B);

  return ret;
}

static TValue * branch_eval(bean_State * B, struct expr * expression) {
  expr * condition = expression->branch.condition;
  dynamic_expr * body;
  enter_scope(B);

  TValue * cond = eval(B, condition);

  if (truthvalue(cond)) {
    body = expression->branch.if_body;
  } else {
    body = expression->branch.else_body;
  }

  TValue * retVal = NULL;

  if (body) {
    for (int i = 0; i < body->count; i++) {
      expr * ep = body->es[i];
      retVal = eval(B, ep);

      if (call_stack_peek(B)) break;
    }
  } else {
    retVal = G(B) -> nil;
  }

  leave_scope(B);
  return retVal;
}

static TValue * regex_eval(bean_State * B UNUSED, struct expr * expression) {
  char * matchStr = getstr(expression->regex.match);
  TValue * ret = init_regex(B, matchStr, ""); // TODO: Not support mode in syntax sugar
  return ret;
}

static TValue * string_eval(bean_State * B UNUSED, struct expr * expression) {
  TValue * ret = TV_MALLOC;
  TString * str = expression->sval;
  setsvalue(ret, str);
  return ret;
}

static TValue * array_eval(bean_State * B UNUSED, struct expr * expression) {
  TValue * ret = TV_MALLOC;
  dynamic_expr * eps = expression->array;
  Array * array = init_array(B);

  for (int i = 0; i < eps->count; i++) {
    TValue * val = eval(B, eps->es[i]);
    array_push(B, array, val);
  }
  setarrvalue(ret, array);
  return ret;
}

static TValue * hash_key_eval(bean_State * B UNUSED, struct expr * expression) {
  switch(expression->type) {
    case(EXPR_GVAR): {
      TString * name = expression->var.name;
      TValue * n = TV_MALLOC;
      setsvalue(n, name);
      return n;
    }
    case(EXPR_STRING): {
      TString * name = expression->sval;
      TValue * str = TV_MALLOC;
      setsvalue(str, name);
      return str;
    }
    default: {
      eval_error(B, "%s", "Invalid key.");
      return G(B)->nil;
    }
  }
}

static TValue * hash_eval(bean_State * B UNUSED, struct expr * expression) {
  TValue * ret = TV_MALLOC;
  dynamic_expr * eps = expression->hash;
  Hash * hash = init_hash(B);
  assert(eps->count % 2 == 0);
  for (int i = 0; i < eps->count; i+=2) {
    TValue * key = hash_key_eval(B, eps->es[i]);
    TValue * value = eval(B, eps->es[i + 1]);

    if (ttisfunction(value) && !fcvalue(value)->p->name) {
      fcvalue(value)->p->name = svalue(key);
    }

    hash_set(B, hash, key, value);
  }
  sethashvalue(ret, hash);
  return ret;
}

eval_func fn[] = {
   nil_eval,
   number_eval,
   boolean_eval,
   unary_eval,
   change_eval,
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
   hash_eval,
   regex_eval,
   break_eval
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
  G -> netproto = init_Http(B);

  G -> nproto -> prototype = G -> meta;
  G -> sproto -> prototype = G -> meta;
  G -> aproto -> prototype = G -> meta;
  G -> hproto -> prototype = G -> meta;
  G -> mproto -> prototype = G -> meta;
  G -> rproto -> prototype = G -> meta;
  G -> dproto -> prototype = G -> meta;
  G -> fproto -> prototype = G -> meta;
  G -> netproto -> prototype = G -> meta;

  G -> meta -> prototype = G->nil;

  add_tools(B); // Depend on the previous setting

  TValue * self = TV_MALLOC;
  setsvalue(self, beanS_newlstr(B, "self", 4));
  hash_set(B, G->globalScope->variables, self, G->nil);

  G -> tVal = TV_MALLOC;
  setbvalue(G->tVal, true);
  G -> fVal = TV_MALLOC;
  setbvalue(G->fVal, false);
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

TValue * eval(bean_State * B, struct expr * expression) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression);
}
