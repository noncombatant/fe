// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>

#include "fe.h"

#define CAR(x) ((x)->car.o)
#define CDR(x) ((x)->cdr.o)
#define TAG(x) ((x)->car.c)
#define IS_NIL(x) ((x) == &nil)
#define TYPE(x) (TAG(x) & 0x1 ? TAG(x) >> 2 : FE_TPAIR)
// TODO: When we use a real enum for type, update this.
#define SET_TYPE(x, t) (TAG(x) = (char)((t) << 2 | 1))
#define NUMBER(x) ((x)->cdr.n)
#define PRIM(x) ((x)->cdr.c)
#define NATIVE_FN(x) ((x)->cdr.f)
#define STRING_BUFFER(x) (&(x)->car.c + 1)

#define STRING_BUFFER_SIZE (sizeof(FeObject*) - 1)
#define GC_MARK_BIT (0x2)
// TODO: This should scale with arena size.
#define GC_STACK_SIZE (512)

enum {
  P_LET,
  P_SET,
  P_IF,
  P_FN,
  P_MAC,
  P_WHILE,
  P_QUOTE,
  P_AND,
  P_OR,
  P_DO,
  P_CONS,
  P_CAR,
  P_CDR,
  P_SETCAR,
  P_SETCDR,
  P_LIST,
  P_NOT,
  P_IS,
  P_ATOM,
  P_PRINT,
  P_LT,
  P_LTE,
  P_ADD,
  P_SUB,
  P_MUL,
  P_DIV,
  P_MAX
};

static const char* primitive_names[] = {
    "let",  "=",     "if",  "fn",  "mac",    "while",  "quote", "and", "or",
    "do",   "cons",  "car", "cdr", "setcar", "setcdr", "list",  "not", "is",
    "atom", "print", "<",   "<=",  "+",      "-",      "*",     "/"};

static const char* type_names[] = {"pair",   "free",      "nil", "number",
                                   "symbol", "string",    "fn",  "macro",
                                   "prim",   "native-fn", "ptr"};

typedef union {
  FeObject* o;
  FeNativeFn* f;
  FeNumber n;
  char c;
} Value;

struct FeObject {
  Value car, cdr;
};

struct FeContext {
  FeHandlers handlers;
  FeObject* gcstack[GC_STACK_SIZE];
  size_t gc_stack_index;
  FeObject* objects;
  size_t object_count;
  FeObject* call_list;
  FeObject* free_list;
  FeObject* symbol_list;
  FeObject* t;
  char nextchr;
};

static FeObject nil = {{(void*)(FE_TNIL << 2 | 1)}, {NULL}};

FeHandlers* FeGetHandlers(FeContext* ctx) {
  return &ctx->handlers;
}

static void Format(char* result, size_t size, const char* format, ...)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgcc-compat"
    __attribute((format(printf, 3, 4))) {
#pragma clang diagnostic pop
  assert(size < INT_MAX);
  assert(size > 0);
  assert(result != NULL);
  assert(format != NULL);

  va_list arguments;
  va_start(arguments, format);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
  const int count = vsnprintf(result, size, format, arguments);
#pragma clang diagnostic pop
  va_end(arguments);
  assert(count > 0);
  assert(count < INT_MAX);
}

void noreturn FeHandleError(FeContext* ctx, const char* msg) {
  FeObject* cl = ctx->call_list;
  // reset context state:
  ctx->call_list = &nil;

  if (ctx->handlers.error) {
    ctx->handlers.error(ctx, msg, cl);
  }
  fprintf(stderr, "error: %s\n", msg);
  for (; !IS_NIL(cl); cl = CDR(cl)) {
    char buf[64];
    FeToString(ctx, CAR(cl), buf, sizeof(buf));
    fprintf(stderr, "=> %s\n", buf);
  }
  exit(EXIT_FAILURE);
}

FeObject* FeGetNextArgument(FeContext* ctx, FeObject** arg) {
  FeObject* a = *arg;
  if (TYPE(a) != FE_TPAIR) {
    if (IS_NIL(a)) {
      FeHandleError(ctx, "too few arguments");
    }
    FeHandleError(ctx, "dotted pair in argument list");
  }
  *arg = CDR(a);
  return CAR(a);
}

static FeObject* CheckType(FeContext* ctx, FeObject* obj, int type) {
  if (TYPE(obj) != type) {
    char message[64];
    Format(message, sizeof(message), "expected %s, got %s", type_names[type],
           type_names[TYPE(obj)]);
    FeHandleError(ctx, message);
  }
  return obj;
}

int FeGetType(FeContext*, FeObject* obj) {
  return TYPE(obj);
}

bool FeIsNil(FeContext*, FeObject* obj) {
  return IS_NIL(obj);
}

void FePushGC(FeContext* ctx, FeObject* obj) {
  if (ctx->gc_stack_index == GC_STACK_SIZE) {
    FeHandleError(ctx, "GC stack overflow");
  }
  ctx->gcstack[ctx->gc_stack_index++] = obj;
}

void FeRestoreGC(FeContext* ctx, size_t index) {
  ctx->gc_stack_index = index;
}

size_t FeSaveGC(FeContext* ctx) {
  return ctx->gc_stack_index;
}

void FeMark(FeContext* ctx, FeObject* obj) {
  FeObject* car;
begin:
  if (TAG(obj) & GC_MARK_BIT) {
    return;
  }
  car = CAR(obj); /* store car before modifying it with GC_MARK_BIT */
  TAG(obj) |= GC_MARK_BIT;

  switch (TYPE(obj)) {
    case FE_TPAIR:
      FeMark(ctx, car);
      /* fall through */
    case FE_TFN:
    case FE_TMACRO:
    case FE_TSYMBOL:
    case FE_TSTRING:
      obj = CDR(obj);
      goto begin;

    case FE_TPTR:
      if (ctx->handlers.mark) {
        ctx->handlers.mark(ctx, obj);
      }
      break;
  }
}

static void CollectGarbage(FeContext* ctx) {
  // Mark:
  for (size_t i = 0; i < ctx->gc_stack_index; i++) {
    FeMark(ctx, ctx->gcstack[i]);
  }
  FeMark(ctx, ctx->symbol_list);

  // Sweep and unmark:
  for (size_t i = 0; i < ctx->object_count; i++) {
    FeObject* obj = &ctx->objects[i];
    if (TYPE(obj) == FE_TFREE) {
      continue;
    }
    if (~TAG(obj) & GC_MARK_BIT) {
      if (TYPE(obj) == FE_TPTR && ctx->handlers.gc) {
        ctx->handlers.gc(ctx, obj);
      }
      SET_TYPE(obj, FE_TFREE);
      CDR(obj) = ctx->free_list;
      ctx->free_list = obj;
    } else {
      TAG(obj) &= ~GC_MARK_BIT;
    }
  }
}

// Translated from [the original
// Java](https://floating-point-gui.de/errors/comparison/).
//
// See also
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/.
static bool IsNearlyEqual(double a, double b, double epsilon) {
  const double absA = fabs(a);
  const double absB = fabs(b);
  const double diff = fabs(a - b);
// It's OK to turn this warning off for this limited section of code, because
// it's in the context of handling tiny errors.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
  if (a == b) {
    // Special case; handles infinities.
    return true;
  } else if (a == 0 || b == 0 || (absA + absB < DBL_MIN)) {
#pragma clang diagnostic pop
    // Either a or b is zero, or both are extremely close to it. Relative error
    // is less meaningful here.
    return diff < (epsilon * DBL_MIN);
  }
  // Use relative error.
  return diff / fmin((absA + absB), DBL_MAX) < epsilon;
}

static bool Equal(FeObject* a, FeObject* b) {
  if (a == b) {
    return true;
  }
  if (TYPE(a) != TYPE(b)) {
    return false;
  }
  if (TYPE(a) == FE_TNUMBER) {
    return IsNearlyEqual(NUMBER(a), NUMBER(b), DBL_EPSILON);
  }
  if (TYPE(a) == FE_TSTRING) {
    for (; !IS_NIL(a); a = CDR(a), b = CDR(b)) {
      if (CAR(a) != CAR(b)) {
        return false;
      }
    }
    return a == b;
  }
  return false;
}

static int IsStringEqual(FeObject* obj, const char* str) {
  while (!IS_NIL(obj)) {
    for (size_t i = 0; i < STRING_BUFFER_SIZE; i++) {
      if (STRING_BUFFER(obj)[i] != *str) {
        return 0;
      }
      if (*str) {
        str++;
      }
    }
    obj = CDR(obj);
  }
  return *str == '\0';
}

static FeObject* MakeObject(FeContext* ctx) {
  // Run GC if free_list has no more objects:
  if (IS_NIL(ctx->free_list)) {
    CollectGarbage(ctx);
    if (IS_NIL(ctx->free_list)) {
      FeHandleError(ctx, "out of memory");
    }
  }
  // Get object from free_list and push it onto the GC stack:
  FeObject* obj = ctx->free_list;
  ctx->free_list = CDR(obj);
  FePushGC(ctx, obj);
  return obj;
}

FeObject* FeCons(FeContext* ctx, FeObject* car, FeObject* cdr) {
  FeObject* obj = MakeObject(ctx);
  CAR(obj) = car;
  CDR(obj) = cdr;
  return obj;
}

FeObject* FeMakeBool(FeContext* ctx, bool b) {
  return b ? ctx->t : &nil;
}

FeObject* FeMakeNumber(FeContext* ctx, FeNumber n) {
  FeObject* obj = MakeObject(ctx);
  SET_TYPE(obj, FE_TNUMBER);
  NUMBER(obj) = n;
  return obj;
}

static FeObject* BuildString(FeContext* ctx, FeObject* tail, char chr) {
  if (!tail || STRING_BUFFER(tail)[STRING_BUFFER_SIZE - 1] != '\0') {
    FeObject* obj = FeCons(ctx, NULL, &nil);
    SET_TYPE(obj, FE_TSTRING);
    if (tail) {
      CDR(tail) = obj;
      ctx->gc_stack_index--;
    }
    tail = obj;
  }
  STRING_BUFFER(tail)[strlen(STRING_BUFFER(tail))] = chr;
  return tail;
}

FeObject* FeMakeString(FeContext* ctx, const char* str) {
  FeObject* obj = BuildString(ctx, NULL, '\0');
  FeObject* tail = obj;
  while (*str) {
    tail = BuildString(ctx, tail, *str++);
  }
  return obj;
}

FeObject* FeMakeSymbol(FeContext* ctx, const char* name) {
  FeObject* obj;
  /* try to find in symbol_list */
  for (obj = ctx->symbol_list; !IS_NIL(obj); obj = CDR(obj)) {
    if (IsStringEqual(CAR(CDR(CAR(obj))), name)) {
      return CAR(obj);
    }
  }
  /* create new object, push to symbol_list and return */
  obj = MakeObject(ctx);
  SET_TYPE(obj, FE_TSYMBOL);
  CDR(obj) = FeCons(ctx, FeMakeString(ctx, name), &nil);
  ctx->symbol_list = FeCons(ctx, obj, ctx->symbol_list);
  return obj;
}

FeObject* FeMakeNativeFn(FeContext* ctx, FeNativeFn fn) {
  FeObject* obj = MakeObject(ctx);
  SET_TYPE(obj, FE_TNATIVE_FN);
  NATIVE_FN(obj) = fn;
  return obj;
}

FeObject* FeMakePtr(FeContext* ctx, void* ptr) {
  FeObject* obj = MakeObject(ctx);
  SET_TYPE(obj, FE_TPTR);
  CDR(obj) = ptr;
  return obj;
}

FeObject* FeMakeList(FeContext* ctx, FeObject** objs, size_t n) {
  FeObject* res = &nil;
  while (n--) {
    res = FeCons(ctx, objs[n], res);
  }
  return res;
}

FeObject* FeCar(FeContext* ctx, FeObject* obj) {
  if (IS_NIL(obj)) {
    return obj;
  }
  return CAR(CheckType(ctx, obj, FE_TPAIR));
}

FeObject* FeCdr(FeContext* ctx, FeObject* obj) {
  if (IS_NIL(obj)) {
    return obj;
  }
  return CDR(CheckType(ctx, obj, FE_TPAIR));
}

static void WriteString(FeContext* ctx,
                        FeWriteFn fn,
                        void* udata,
                        const char* s) {
  while (*s) {
    fn(ctx, udata, *s++);
  }
}

void FeWrite(FeContext* ctx, FeObject* obj, FeWriteFn fn, void* udata, int qt) {
  char buf[32];
  switch (TYPE(obj)) {
    case FE_TNIL:
      WriteString(ctx, fn, udata, "nil");
      break;

    case FE_TNUMBER:
      Format(buf, sizeof(buf), "%.7g", NUMBER(obj));
      WriteString(ctx, fn, udata, buf);
      break;

    case FE_TPAIR:
      fn(ctx, udata, '(');
      while (true) {
        FeWrite(ctx, CAR(obj), fn, udata, 1);
        obj = CDR(obj);
        if (TYPE(obj) != FE_TPAIR) {
          break;
        }
        fn(ctx, udata, ' ');
      }
      if (!IS_NIL(obj)) {
        WriteString(ctx, fn, udata, " . ");
        FeWrite(ctx, obj, fn, udata, 1);
      }
      fn(ctx, udata, ')');
      break;

    case FE_TSYMBOL:
      FeWrite(ctx, CAR(CDR(obj)), fn, udata, 0);
      break;

    case FE_TSTRING:
      if (qt) {
        fn(ctx, udata, '"');
      }
      while (!IS_NIL(obj)) {
        for (size_t i = 0; i < STRING_BUFFER_SIZE && STRING_BUFFER(obj)[i];
             i++) {
          if (qt && STRING_BUFFER(obj)[i] == '"') {
            fn(ctx, udata, '\\');
          }
          fn(ctx, udata, STRING_BUFFER(obj)[i]);
        }
        obj = CDR(obj);
      }
      if (qt) {
        fn(ctx, udata, '"');
      }
      break;

    default:
      Format(buf, sizeof(buf), "[%s %p]", type_names[TYPE(obj)], (void*)obj);
      WriteString(ctx, fn, udata, buf);
      break;
  }
}

// TODO: See if `void*` is really necessary here, in `WriteBuffer`, et c.
static void WriteFile(FeContext*, void* udata, char chr) {
  fputc(chr, udata);
}

void FeWriteFile(FeContext* ctx, FeObject* obj, FILE* fp) {
  FeWrite(ctx, obj, WriteFile, fp, 0);
}

typedef struct {
  char* p;
  size_t n;
} CharPtrInt;  // TODO: Not the clearest name

static void WriteBuffer(FeContext*, void* udata, char chr) {
  CharPtrInt* x = udata;
  if (x->n) {
    *x->p++ = chr;
    x->n--;
  }
}

size_t FeToString(FeContext* ctx, FeObject* obj, char* dst, size_t size) {
  CharPtrInt x = {.p = dst, .n = size - 1};
  FeWrite(ctx, obj, WriteBuffer, &x, 0);
  *x.p = '\0';
  return size - x.n - 1;
}

FeNumber FeToNumber(FeContext* ctx, FeObject* obj) {
  return NUMBER(CheckType(ctx, obj, FE_TNUMBER));
}

void* FeToPtr(FeContext* ctx, FeObject* obj) {
  return CDR(CheckType(ctx, obj, FE_TPTR));
}

static FeObject* GetBound(FeObject* sym, FeObject* env) {
  // Try to find the symbol in the environment:
  for (; !IS_NIL(env); env = CDR(env)) {
    FeObject* x = CAR(env);
    if (CAR(x) == sym) {
      return x;
    }
  }
  // Otherwise, return a global value:
  return CDR(sym);
}

void FeSet(FeContext*, FeObject* sym, FeObject* v) {
  CDR(GetBound(sym, &nil)) = v;
}

static FeObject rparen;

static FeObject* Read(FeContext* ctx, FeReadFn fn, void* udata) {
  /* get next character */
  char chr = ctx->nextchr ? ctx->nextchr : fn(ctx, udata);
  ctx->nextchr = '\0';

  /* skip whitespace */
  while (chr && strchr(" \n\t\r", chr)) {
    chr = fn(ctx, udata);
  }

  switch (chr) {
    case '\0':
      return NULL;

    case ';':
      while (chr && chr != '\n') {
        chr = fn(ctx, udata);
      }
      return Read(ctx, fn, udata);

    case ')':
      return &rparen;

    case '(': {
      FeObject* res = &nil;
      FeObject** tail = &res;
      size_t gc = FeSaveGC(ctx);
      FePushGC(ctx, res); /* to cause error on too-deep nesting */
      FeObject* v;
      while ((v = Read(ctx, fn, udata)) != &rparen) {
        if (v == NULL) {
          FeHandleError(ctx, "unclosed list");
        }
        if (TYPE(v) == FE_TSYMBOL && IsStringEqual(CAR(CDR(v)), ".")) {
          /* dotted pair */
          *tail = FeRead(ctx, fn, udata);
        } else {
          /* proper pair */
          *tail = FeCons(ctx, v, &nil);
          tail = &CDR(*tail);
        }
        FeRestoreGC(ctx, gc);
        FePushGC(ctx, res);
      }
      return res;
    }

    case '\'': {
      FeObject* v = FeRead(ctx, fn, udata);
      if (v == NULL) {
        FeHandleError(ctx, "stray '''");
      }
      return FeCons(ctx, FeMakeSymbol(ctx, "quote"), FeCons(ctx, v, &nil));
    }

    case '"': {
      FeObject* res = BuildString(ctx, NULL, '\0');
      FeObject* v = res;
      chr = fn(ctx, udata);
      while (chr != '"') {
        if (chr == '\0') {
          FeHandleError(ctx, "unclosed string");
        }
        if (chr == '\\') {
          chr = fn(ctx, udata);
          if (strchr("nrt", chr)) {
            chr = strchr("n\nr\rt\t", chr)[1];
          }
        }
        v = BuildString(ctx, v, chr);
        chr = fn(ctx, udata);
      }
      return res;
    }

    default: {
      char buf[64];
      char* p = buf;
      const char* delimiter = " \n\t\r();";
      do {
        if (p == buf + sizeof(buf) - 1) {
          FeHandleError(ctx, "symbol too long");
        }
        *p++ = chr;
        chr = fn(ctx, udata);
      } while (chr && !strchr(delimiter, chr));
      *p = '\0';
      ctx->nextchr = chr;
      // Try to read it as a number:
      FeNumber n = strtod(buf, &p);
      if (p != buf && strchr(delimiter, *p)) {
        return FeMakeNumber(ctx, n);
      }
      // Try to read it as nil:
      if (!strcmp(buf, "nil")) {
        return &nil;
      }
      // It's a symbol:
      return FeMakeSymbol(ctx, buf);
    }
  }
}

FeObject* FeRead(FeContext* ctx, FeReadFn fn, void* udata) {
  FeObject* obj = Read(ctx, fn, udata);
  if (obj == &rparen) {
    FeHandleError(ctx, "stray ')'");
  }
  return obj;
}

static char ReadFile(FeContext*, void* udata) {
  int c = fgetc(udata);
  return c == EOF ? '\0' : (char)c;
}

FeObject* FeReadFile(FeContext* ctx, FILE* fp) {
  return FeRead(ctx, ReadFile, fp);
}

static FeObject* Evaluate(FeContext* ctx,
                          FeObject* obj,
                          FeObject* env,
                          FeObject** bind);

static FeObject* EvaluateList(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  FeObject** tail = &res;
  while (!IS_NIL(lst)) {
    *tail = FeCons(ctx, Evaluate(ctx, FeGetNextArgument(ctx, &lst), env, NULL),
                   &nil);
    tail = &CDR(*tail);
  }
  return res;
}

static FeObject* DoList(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  size_t save = FeSaveGC(ctx);
  while (!IS_NIL(lst)) {
    FeRestoreGC(ctx, save);
    FePushGC(ctx, lst);
    FePushGC(ctx, env);
    res = Evaluate(ctx, FeGetNextArgument(ctx, &lst), env, &env);
  }
  return res;
}

static FeObject* ArgsToEnv(FeContext* ctx,
                           FeObject* prm,
                           FeObject* arg,
                           FeObject* env) {
  while (!IS_NIL(prm)) {
    if (TYPE(prm) != FE_TPAIR) {
      env = FeCons(ctx, FeCons(ctx, prm, arg), env);
      break;
    }
    env = FeCons(ctx, FeCons(ctx, CAR(prm), FeCar(ctx, arg)), env);
    prm = CDR(prm);
    arg = FeCdr(ctx, arg);
  }
  return env;
}

#define EVAL_ARG() Evaluate(ctx, FeGetNextArgument(ctx, &arg), env, NULL)

#define ARITH_OP(op)                          \
  {                                           \
    FeNumber x = FeToNumber(ctx, EVAL_ARG()); \
    while (!IS_NIL(arg)) {                    \
      x = x op FeToNumber(ctx, EVAL_ARG());   \
    }                                         \
    res = FeMakeNumber(ctx, x);               \
  }

#define NUM_CMP_OP(op)                               \
  {                                                  \
    va = CheckType(ctx, EVAL_ARG(), FE_TNUMBER);     \
    vb = CheckType(ctx, EVAL_ARG(), FE_TNUMBER);     \
    res = FeMakeBool(ctx, NUMBER(va) op NUMBER(vb)); \
  }

static FeObject* Evaluate(FeContext* ctx,
                          FeObject* obj,
                          FeObject* env,
                          FeObject** newenv) {
  if (TYPE(obj) == FE_TSYMBOL) {
    return CDR(GetBound(obj, env));
  }
  if (TYPE(obj) != FE_TPAIR) {
    return obj;
  }

  FeObject cl;
  CAR(&cl) = obj;
  CDR(&cl) = ctx->call_list;
  ctx->call_list = &cl;

  size_t gc = FeSaveGC(ctx);
  FeObject* fn = Evaluate(ctx, CAR(obj), env, NULL);
  FeObject* arg = CDR(obj);
  FeObject* res = &nil;
  FeObject* va;
  FeObject* vb;

  switch (TYPE(fn)) {
    case FE_TPRIM:
      switch (PRIM(fn)) {
        case P_LET:
          va = CheckType(ctx, FeGetNextArgument(ctx, &arg), FE_TSYMBOL);
          if (newenv) {
            *newenv = FeCons(ctx, FeCons(ctx, va, EVAL_ARG()), env);
          }
          break;

        case P_SET:
          va = CheckType(ctx, FeGetNextArgument(ctx, &arg), FE_TSYMBOL);
          CDR(GetBound(va, env)) = EVAL_ARG();
          break;

        case P_IF:
          while (!IS_NIL(arg)) {
            va = EVAL_ARG();
            if (!IS_NIL(va)) {
              res = IS_NIL(arg) ? va : EVAL_ARG();
              break;
            }
            if (IS_NIL(arg)) {
              break;
            }
            arg = CDR(arg);
          }
          break;

        case P_FN:
        case P_MAC:
          va = FeCons(ctx, env, arg);
          FeGetNextArgument(ctx, &arg);
          res = MakeObject(ctx);
          SET_TYPE(res, PRIM(fn) == P_FN ? FE_TFN : FE_TMACRO);
          CDR(res) = va;
          break;

        case P_WHILE: {
          va = FeGetNextArgument(ctx, &arg);
          size_t n = FeSaveGC(ctx);
          while (!IS_NIL(Evaluate(ctx, va, env, NULL))) {
            DoList(ctx, arg, env);
            FeRestoreGC(ctx, n);
          }
          break;
        }

        case P_QUOTE:
          res = FeGetNextArgument(ctx, &arg);
          break;

        case P_AND:
          while (!IS_NIL(arg) && !IS_NIL(res = EVAL_ARG()))
            ;
          break;

        case P_OR:
          while (!IS_NIL(arg) && IS_NIL(res = EVAL_ARG()))
            ;
          break;

        case P_DO:
          res = DoList(ctx, arg, env);
          break;

        case P_CONS:
          va = EVAL_ARG();
          res = FeCons(ctx, va, EVAL_ARG());
          break;

        case P_CAR:
          res = FeCar(ctx, EVAL_ARG());
          break;

        case P_CDR:
          res = FeCdr(ctx, EVAL_ARG());
          break;

        case P_SETCAR:
          va = CheckType(ctx, EVAL_ARG(), FE_TPAIR);
          CAR(va) = EVAL_ARG();
          break;

        case P_SETCDR:
          va = CheckType(ctx, EVAL_ARG(), FE_TPAIR);
          CDR(va) = EVAL_ARG();
          break;

        case P_LIST:
          res = EvaluateList(ctx, arg, env);
          break;

        case P_NOT:
          res = FeMakeBool(ctx, IS_NIL(EVAL_ARG()));
          break;

        case P_IS:
          va = EVAL_ARG();
          res = FeMakeBool(ctx, Equal(va, EVAL_ARG()));
          break;

        case P_ATOM:
          res = FeMakeBool(ctx, FeGetType(ctx, EVAL_ARG()) != FE_TPAIR);
          break;

        case P_PRINT:
          while (!IS_NIL(arg)) {
            FeWriteFile(ctx, EVAL_ARG(), stdout);
            if (!IS_NIL(arg)) {
              printf(" ");
            }
          }
          printf("\n");
          break;

        case P_LT:
          NUM_CMP_OP(<);
          break;
        case P_LTE:
          NUM_CMP_OP(<=);
          break;
        case P_ADD:
          ARITH_OP(+);
          break;
        case P_SUB:
          ARITH_OP(-);
          break;
        case P_MUL:
          ARITH_OP(*);
          break;
        case P_DIV:
          ARITH_OP(/);
          break;
      }
      break;

    case FE_TNATIVE_FN:
      res = NATIVE_FN(fn)(ctx, EvaluateList(ctx, arg, env));
      break;

    case FE_TFN:
      arg = EvaluateList(ctx, arg, env);
      va = CDR(fn); /* (env params ...) */
      vb = CDR(va); /* (params ...) */
      res = DoList(ctx, CDR(vb), ArgsToEnv(ctx, CAR(vb), arg, CAR(va)));
      break;

    case FE_TMACRO:
      va = CDR(fn); /* (env params ...) */
      vb = CDR(va); /* (params ...) */
      /* replace caller object with code generated by macro and re-eval */
      *obj = *DoList(ctx, CDR(vb), ArgsToEnv(ctx, CAR(vb), arg, CAR(va)));
      FeRestoreGC(ctx, gc);
      ctx->call_list = CDR(&cl);
      return Evaluate(ctx, obj, env, NULL);

    default:
      FeHandleError(ctx, "tried to call non-callable value");
  }

  FeRestoreGC(ctx, gc);
  FePushGC(ctx, res);
  ctx->call_list = CDR(&cl);
  return res;
}

FeObject* FeEvaluate(FeContext* ctx, FeObject* obj) {
  return Evaluate(ctx, obj, &nil, NULL);
}

FeContext* FeOpenContext(void* arena, size_t size) {
  // Initialize the context:
  FeContext* ctx = arena;
  memset(ctx, 0, sizeof(FeContext));
  arena = (char*)arena + sizeof(FeContext);
  size -= sizeof(FeContext);

  // Initialize the objects memory region:
  ctx->objects = (FeObject*)arena;
  ctx->object_count = size / sizeof(FeObject);

  // Initialize the lists:
  ctx->call_list = &nil;
  ctx->free_list = &nil;
  ctx->symbol_list = &nil;

  // Populate the free_list:
  for (size_t i = 0; i < ctx->object_count; i++) {
    FeObject* obj = &ctx->objects[i];
    SET_TYPE(obj, FE_TFREE);
    CDR(obj) = ctx->free_list;
    ctx->free_list = obj;
  }

  // Initialize the objects:
  ctx->t = FeMakeSymbol(ctx, "t");
  FeSet(ctx, ctx->t, ctx->t);

  // Register the built-in primitives:
  size_t save = FeSaveGC(ctx);
  for (size_t i = 0; i < P_MAX; i++) {
    FeObject* v = MakeObject(ctx);
    SET_TYPE(v, FE_TPRIM);
    PRIM(v) = (char)i;
    FeSet(ctx, FeMakeSymbol(ctx, primitive_names[i]), v);
    FeRestoreGC(ctx, save);
  }
  return ctx;
}

void FeCloseContext(FeContext* ctx) {
  // Clear the GC stack and symbol list: this makes all objects unreachable:
  ctx->gc_stack_index = 0;
  ctx->symbol_list = &nil;
  CollectGarbage(ctx);
}
