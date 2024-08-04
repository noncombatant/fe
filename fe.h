// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FE_H
#define FE_H

#include <stdio.h>
#include <stdlib.h>

#define FE_VERSION "1.0"

typedef double FeNumber;
typedef struct FeObject FeObject;
typedef struct FeContext FeContext;
typedef FeObject* FeCFunc(FeContext* ctx, FeObject* args);
typedef void FeErrorFn(FeContext* ctx, const char* err, FeObject* cl);
typedef void FeWriteFn(FeContext* ctx, void* udata, char chr);
typedef char FeReadFn(FeContext* ctx, void* udata);

typedef struct {
  FeErrorFn* error;
  FeCFunc* mark;
  FeCFunc* gc;
} FeHandlers;

enum {
  FE_TPAIR,
  FE_TFREE,
  FE_TNIL,
  FE_TNUMBER,
  FE_TSYMBOL,
  FE_TSTRING,
  FE_TFUNC,
  FE_TMACRO,
  FE_TPRIM,
  FE_TCFUNC,
  FE_TPTR
};

FeContext* FeOpenContext(void* ptr, size_t size);
void FeCloseContext(FeContext* ctx);
FeHandlers* FeGetHandlers(FeContext* ctx);

void FeHandleError(FeContext* ctx, const char* msg);
FeObject* FeGetNextArgument(FeContext* ctx, FeObject** arg);

int FeGetType(FeContext* ctx, FeObject* obj);  // TODO enum return
int FeIsNil(FeContext* ctx, FeObject* obj);    // TODO bool

void FePushGC(FeContext* ctx, FeObject* obj);
void FeRestoreGC(FeContext* ctx, size_t idx);
size_t FeSaveGC(FeContext* ctx);
void FeMark(FeContext* ctx, FeObject* obj);

FeObject* FeCons(FeContext* ctx, FeObject* car, FeObject* cdr);
FeObject* FeMakeBool(FeContext* ctx, int b);  // TODO use `bool`
FeObject* FeMakeNumber(FeContext* ctx, FeNumber n);
FeObject* FeMakeString(FeContext* ctx, const char* str);
FeObject* FeMakeSymbol(FeContext* ctx, const char* name);
FeObject* FeMakeCFunc(FeContext* ctx, FeCFunc fn);
FeObject* FeMakePtr(FeContext* ctx, void* ptr);
FeObject* FeMakeList(FeContext* ctx, FeObject** objs, int n);

FeObject* FeCar(FeContext* ctx, FeObject* obj);
FeObject* FeCdr(FeContext* ctx, FeObject* obj);

void FeWrite(FeContext* ctx, FeObject* obj, FeWriteFn fn, void* udata, int qt);
void FeWriteFile(FeContext* ctx, FeObject* obj, FILE* fp);

FeObject* FeRead(FeContext* ctx, FeReadFn fn, void* udata);
FeObject* FeReadFile(FeContext* ctx, FILE* fp);

int FeToString(FeContext* ctx,
               FeObject* obj,
               char* dst,
               int size);  // TODO size_t
FeNumber FeToNumber(FeContext* ctx, FeObject* obj);
void* FeToPtr(FeContext* ctx, FeObject* obj);
void FeSet(FeContext* ctx, FeObject* sym, FeObject* v);

FeObject* FeEvaluate(FeContext* ctx, FeObject* obj);

#endif
