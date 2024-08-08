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

#define STRING_BUFFER_SIZE (sizeof(FeObject*) - 1)
#define GC_MARK_BIT (0x2)
// TODO: This should scale with arena size.
#define GC_STACK_SIZE (512)

typedef enum Primitive {
  PLet,
  PSet,
  PIf,
  PFn,
  PMacro,
  PWhile,
  PQuote,
  PAnd,
  POr,
  PDo,
  PCons,
  PCar,
  PCdr,
  PSetCar,
  PSetCdr,
  PList,
  PNot,
  PIs,
  PAtom,
  PPrint,
  PLess,
  PLessEqual,
  PAdd,
  PSub,
  PMul,
  PDiv,
  PSentinel
} Primitive;

static const char* primitive_names[] = {
    "let",  "=",     "if",  "fn",  "macro",  "while",  "quote", "and", "or",
    "do",   "cons",  "car", "cdr", "setcar", "setcdr", "list",  "not", "is",
    "atom", "print", "<",   "<=",  "+",      "-",      "*",     "/"};

static const char* type_names[] = {"pair",   "free",      "nil", "double",
                                   "symbol", "string",    "fn",  "macro",
                                   "prim",   "native-fn", "ptr"};

typedef union {
  FeObject* o;
  FeNativeFn* f;
  FeDouble n;
  char c;
} Value;

struct FeObject {
  Value car, cdr;
};

FeObject nil = {{(void*)(FeTNil << 2 | 1)}, {NULL}};

#define CAR(x) ((x)->car.o)
#define CDR(x) ((x)->cdr.o)
#define TAG(x) ((x)->car.c)
#define DOUBLE(x) ((x)->cdr.n)
#define PRIM(x) ((x)->cdr.c)
#define NATIVE_FN(x) ((x)->cdr.f)
#define STRING_BUFFER(x) (&(x)->car.c + 1)

static FeDouble GetDouble(const FeObject* o) {
  return o->cdr.n;
}

static int GetType(const FeObject* o) {
  return TAG(o) & 0x1 ? TAG(o) >> 2 : FeTPair;
}

static FeNativeFn* GetNativeFn(const FeObject* o) {
  return o->cdr.f;
}

static char GetP(const FeObject* o) {
  return o->cdr.c;
}

static bool IsNil(const FeObject* o) {
  return o == &nil;
}

// TODO: When we use a real enum for type, update this.
static void SetType(FeObject* o, int type) {
  o->car.c = (char)((type) << 2 | 1);
}

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
  for (; !IsNil(cl); cl = CDR(cl)) {
    char buf[64];
    FeToString(ctx, CAR(cl), buf, sizeof(buf));
    fprintf(stderr, "=> %s\n", buf);
  }
  exit(EXIT_FAILURE);
}

FeObject* FeGetNextArgument(FeContext* ctx, FeObject** arg) {
  FeObject* a = *arg;
  if (GetType(a) != FeTPair) {
    if (IsNil(a)) {
      FeHandleError(ctx, "too few arguments");
    }
    FeHandleError(ctx, "dotted pair in argument list");
  }
  *arg = CDR(a);
  return CAR(a);
}

static FeObject* CheckType(FeContext* ctx, FeObject* obj, int type) {
  if (GetType(obj) != type) {
    char message[64];
    Format(message, sizeof(message), "expected %s, got %s", type_names[type],
           type_names[GetType(obj)]);
    FeHandleError(ctx, message);
  }
  return obj;
}

int FeGetType(FeContext*, FeObject* obj) {
  return GetType(obj);
}

bool FeIsNil(FeContext*, FeObject* obj) {
  return IsNil(obj);
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

  switch (GetType(obj)) {
    case FeTPair:
      FeMark(ctx, car);
      /* fall through */
    case FeTFn:
    case FeTMacro:
    case FeTSymbol:
    case FeTString:
      obj = CDR(obj);
      goto begin;

    case FeTPtr:
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
    if (GetType(obj) == FeTFree) {
      continue;
    }
    if (~TAG(obj) & GC_MARK_BIT) {
      if (GetType(obj) == FeTPtr && ctx->handlers.gc) {
        ctx->handlers.gc(ctx, obj);
      }
      SetType(obj, FeTFree);
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
  if (GetType(a) != GetType(b)) {
    return false;
  }
  if (GetType(a) == FeTDouble) {
    return IsNearlyEqual(GetDouble(a), GetDouble(b), DBL_EPSILON);
  }
  if (GetType(a) == FeTString) {
    for (; !IsNil(a); a = CDR(a), b = CDR(b)) {
      if (CAR(a) != CAR(b)) {
        return false;
      }
    }
    return a == b;
  }
  return false;
}

static int IsStringEqual(FeObject* obj, const char* str) {
  while (!IsNil(obj)) {
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
  if (IsNil(ctx->free_list)) {
    CollectGarbage(ctx);
    if (IsNil(ctx->free_list)) {
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

FeObject* FeMakeDouble(FeContext* ctx, FeDouble n) {
  FeObject* obj = MakeObject(ctx);
  SetType(obj, FeTDouble);
  DOUBLE(obj) = n;
  return obj;
}

static FeObject* BuildString(FeContext* ctx, FeObject* tail, char chr) {
  if (!tail || STRING_BUFFER(tail)[STRING_BUFFER_SIZE - 1] != '\0') {
    FeObject* obj = FeCons(ctx, NULL, &nil);
    SetType(obj, FeTString);
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
  for (obj = ctx->symbol_list; !IsNil(obj); obj = CDR(obj)) {
    if (IsStringEqual(CAR(CDR(CAR(obj))), name)) {
      return CAR(obj);
    }
  }
  /* create new object, push to symbol_list and return */
  obj = MakeObject(ctx);
  SetType(obj, FeTSymbol);
  CDR(obj) = FeCons(ctx, FeMakeString(ctx, name), &nil);
  ctx->symbol_list = FeCons(ctx, obj, ctx->symbol_list);
  return obj;
}

FeObject* FeMakeNativeFn(FeContext* ctx, FeNativeFn fn) {
  FeObject* obj = MakeObject(ctx);
  SetType(obj, FeTNativeFn);
  NATIVE_FN(obj) = fn;
  return obj;
}

FeObject* FeMakePtr(FeContext* ctx, void* ptr) {
  FeObject* obj = MakeObject(ctx);
  SetType(obj, FeTPtr);
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
  if (IsNil(obj)) {
    return obj;
  }
  return CAR(CheckType(ctx, obj, FeTPair));
}

FeObject* FeCdr(FeContext* ctx, FeObject* obj) {
  if (IsNil(obj)) {
    return obj;
  }
  return CDR(CheckType(ctx, obj, FeTPair));
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
  switch (GetType(obj)) {
    case FeTNil:
      WriteString(ctx, fn, udata, "nil");
      break;

    case FeTDouble:
      Format(buf, sizeof(buf), "%.7g", GetDouble(obj));
      WriteString(ctx, fn, udata, buf);
      break;

    case FeTPair:
      fn(ctx, udata, '(');
      while (true) {
        FeWrite(ctx, CAR(obj), fn, udata, 1);
        obj = CDR(obj);
        if (GetType(obj) != FeTPair) {
          break;
        }
        fn(ctx, udata, ' ');
      }
      if (!IsNil(obj)) {
        WriteString(ctx, fn, udata, " . ");
        FeWrite(ctx, obj, fn, udata, 1);
      }
      fn(ctx, udata, ')');
      break;

    case FeTSymbol:
      FeWrite(ctx, CAR(CDR(obj)), fn, udata, 0);
      break;

    case FeTString:
      if (qt) {
        fn(ctx, udata, '"');
      }
      while (!IsNil(obj)) {
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
      Format(buf, sizeof(buf), "[%s %p]", type_names[GetType(obj)], (void*)obj);
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

FeDouble FeToDouble(FeContext* ctx, FeObject* obj) {
  return GetDouble(CheckType(ctx, obj, FeTDouble));
}

void* FeToPtr(FeContext* ctx, FeObject* obj) {
  return CDR(CheckType(ctx, obj, FeTPtr));
}

static FeObject* GetBound(FeObject* sym, FeObject* env) {
  // Try to find the symbol in the environment:
  for (; !IsNil(env); env = CDR(env)) {
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
        if (GetType(v) == FeTSymbol && IsStringEqual(CAR(CDR(v)), ".")) {
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
      // Try to read it as a double:
      FeDouble n = strtod(buf, &p);
      if (p != buf && strchr(delimiter, *p)) {
        return FeMakeDouble(ctx, n);
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
  while (!IsNil(lst)) {
    *tail = FeCons(ctx, Evaluate(ctx, FeGetNextArgument(ctx, &lst), env, NULL),
                   &nil);
    tail = &CDR(*tail);
  }
  return res;
}

static FeObject* DoList(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  size_t save = FeSaveGC(ctx);
  while (!IsNil(lst)) {
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
  while (!IsNil(prm)) {
    if (GetType(prm) != FeTPair) {
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
    FeDouble x = FeToDouble(ctx, EVAL_ARG()); \
    while (!IsNil(arg)) {                     \
      x = x op FeToDouble(ctx, EVAL_ARG());   \
    }                                         \
    res = FeMakeDouble(ctx, x);               \
  }

#define NUM_CMP_OP(op)                                     \
  {                                                        \
    va = CheckType(ctx, EVAL_ARG(), FeTDouble);            \
    vb = CheckType(ctx, EVAL_ARG(), FeTDouble);            \
    res = FeMakeBool(ctx, GetDouble(va) op GetDouble(vb)); \
  }

static FeObject* Evaluate(FeContext* ctx,
                          FeObject* obj,
                          FeObject* env,
                          FeObject** newenv) {
  if (GetType(obj) == FeTSymbol) {
    return CDR(GetBound(obj, env));
  }
  if (GetType(obj) != FeTPair) {
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

  switch (GetType(fn)) {
    case FeTPrimitive:
      switch (GetP(fn)) {
        case PLet:
          va = CheckType(ctx, FeGetNextArgument(ctx, &arg), FeTSymbol);
          if (newenv) {
            *newenv = FeCons(ctx, FeCons(ctx, va, EVAL_ARG()), env);
          }
          break;

        case PSet:
          va = CheckType(ctx, FeGetNextArgument(ctx, &arg), FeTSymbol);
          CDR(GetBound(va, env)) = EVAL_ARG();
          break;

        case PIf:
          while (!IsNil(arg)) {
            va = EVAL_ARG();
            if (!IsNil(va)) {
              res = IsNil(arg) ? va : EVAL_ARG();
              break;
            }
            if (IsNil(arg)) {
              break;
            }
            arg = CDR(arg);
          }
          break;

        case PFn:
        case PMacro:
          va = FeCons(ctx, env, arg);
          FeGetNextArgument(ctx, &arg);
          res = MakeObject(ctx);
          SetType(res, GetP(fn) == PFn ? FeTFn : FeTMacro);
          CDR(res) = va;
          break;

        case PWhile: {
          va = FeGetNextArgument(ctx, &arg);
          size_t n = FeSaveGC(ctx);
          while (!IsNil(Evaluate(ctx, va, env, NULL))) {
            DoList(ctx, arg, env);
            FeRestoreGC(ctx, n);
          }
          break;
        }

        case PQuote:
          res = FeGetNextArgument(ctx, &arg);
          break;

        case PAnd:
          while (!IsNil(arg) && !IsNil(res = EVAL_ARG()))
            ;
          break;

        case POr:
          while (!IsNil(arg) && IsNil(res = EVAL_ARG()))
            ;
          break;

        case PDo:
          res = DoList(ctx, arg, env);
          break;

        case PCons:
          va = EVAL_ARG();
          res = FeCons(ctx, va, EVAL_ARG());
          break;

        case PCar:
          res = FeCar(ctx, EVAL_ARG());
          break;

        case PCdr:
          res = FeCdr(ctx, EVAL_ARG());
          break;

        case PSetCar:
          va = CheckType(ctx, EVAL_ARG(), FeTPair);
          CAR(va) = EVAL_ARG();
          break;

        case PSetCdr:
          va = CheckType(ctx, EVAL_ARG(), FeTPair);
          CDR(va) = EVAL_ARG();
          break;

        case PList:
          res = EvaluateList(ctx, arg, env);
          break;

        case PNot:
          res = FeMakeBool(ctx, IsNil(EVAL_ARG()));
          break;

        case PIs:
          va = EVAL_ARG();
          res = FeMakeBool(ctx, Equal(va, EVAL_ARG()));
          break;

        case PAtom:
          res = FeMakeBool(ctx, FeGetType(ctx, EVAL_ARG()) != FeTPair);
          break;

        case PPrint:
          while (!IsNil(arg)) {
            FeWriteFile(ctx, EVAL_ARG(), stdout);
            if (!IsNil(arg)) {
              printf(" ");
            }
          }
          printf("\n");
          break;

        case PLess:
          NUM_CMP_OP(<);
          break;
        case PLessEqual:
          NUM_CMP_OP(<=);
          break;
        case PAdd:
          ARITH_OP(+);
          break;
        case PSub:
          ARITH_OP(-);
          break;
        case PMul:
          ARITH_OP(*);
          break;
        case PDiv:
          ARITH_OP(/);
          break;
      }
      break;

    case FeTNativeFn:
      res = GetNativeFn(fn)(ctx, EvaluateList(ctx, arg, env));
      break;

    case FeTFn:
      arg = EvaluateList(ctx, arg, env);
      va = CDR(fn); /* (env params ...) */
      vb = CDR(va); /* (params ...) */
      res = DoList(ctx, CDR(vb), ArgsToEnv(ctx, CAR(vb), arg, CAR(va)));
      break;

    case FeTMacro:
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
    SetType(obj, FeTFree);
    CDR(obj) = ctx->free_list;
    ctx->free_list = obj;
  }

  // Initialize the objects:
  ctx->t = FeMakeSymbol(ctx, "t");
  FeSet(ctx, ctx->t, ctx->t);

  // Register the built-in primitives:
  size_t save = FeSaveGC(ctx);
  for (size_t i = 0; i < PSentinel; i++) {
    FeObject* v = MakeObject(ctx);
    SetType(v, FeTPrimitive);
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
