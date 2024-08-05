// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FE_H
#define FE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define FE_VERSION "1.0"

typedef double FeNumber;
typedef struct FeObject FeObject;
typedef struct FeContext FeContext;
typedef FeObject* FeNativeFn(FeContext* ctx, FeObject* args);
typedef void FeErrorFn(FeContext* ctx, const char* err, FeObject* cl);
typedef void FeWriteFn(FeContext* ctx, void* udata, char chr);
typedef char FeReadFn(FeContext* ctx, void* udata);

typedef struct {
  FeErrorFn* error;
  FeNativeFn* mark;
  FeNativeFn* gc;
} FeHandlers;

typedef enum FeType {
  FeTPair,
  FeTFree,
  FeTNil,
  FeTNumber,
  FeTSymbol,
  FeTString,
  FeTFn,
  FeTMacro,
  FeTPrimitive,
  FeTNativeFn,
  FeTPtr
} FeType;

FeContext* FeOpenContext(void* ptr, size_t size);
void FeCloseContext(FeContext* ctx);
FeHandlers* FeGetHandlers(FeContext* ctx);

void FeHandleError(FeContext* ctx, const char* msg);
FeObject* FeGetNextArgument(FeContext* ctx, FeObject** arg);

int FeGetType(FeContext* ctx, FeObject* obj);  // TODO enum return
bool FeIsNil(FeContext* ctx, FeObject* obj);

void FePushGC(FeContext* ctx, FeObject* obj);
void FeRestoreGC(FeContext* ctx, size_t idx);
size_t FeSaveGC(FeContext* ctx);
void FeMark(FeContext* ctx, FeObject* obj);

FeObject* FeCons(FeContext* ctx, FeObject* car, FeObject* cdr);
FeObject* FeMakeBool(FeContext* ctx, bool b);
FeObject* FeMakeNumber(FeContext* ctx, FeNumber n);
FeObject* FeMakeString(FeContext* ctx, const char* str);
FeObject* FeMakeSymbol(FeContext* ctx, const char* name);
FeObject* FeMakeNativeFn(FeContext* ctx, FeNativeFn fn);
FeObject* FeMakePtr(FeContext* ctx, void* ptr);
FeObject* FeMakeList(FeContext* ctx, FeObject** objs, size_t n);

FeObject* FeCar(FeContext* ctx, FeObject* obj);
FeObject* FeCdr(FeContext* ctx, FeObject* obj);

void FeWrite(FeContext* ctx, FeObject* obj, FeWriteFn fn, void* udata, int qt);
void FeWriteFile(FeContext* ctx, FeObject* obj, FILE* fp);

FeObject* FeRead(FeContext* ctx, FeReadFn fn, void* udata);
FeObject* FeReadFile(FeContext* ctx, FILE* fp);

size_t FeToString(FeContext* ctx, FeObject* obj, char* dst, size_t size);
FeNumber FeToNumber(FeContext* ctx, FeObject* obj);
void* FeToPtr(FeContext* ctx, FeObject* obj);
void FeSet(FeContext* ctx, FeObject* sym, FeObject* v);

FeObject* FeEvaluate(FeContext* ctx, FeObject* obj);

#endif
