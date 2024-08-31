// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fex.h"
#include "fex_io.h"

void FexInstallIO(FeContext* ctx) {
  FexInstallNativeFn(ctx, "close-file", FexCloseFile);
  FexInstallNativeFn(ctx, "open-file", FexOpenFile);
  FexInstallNativeFn(ctx, "read-file", FexReadFile);
  FexInstallNativeFn(ctx, "write-file", FexWriteFile);

  FeSet(ctx, FeMakeSymbol(ctx, "stdin"), FeMakePtr(ctx, FexTFile, stdin));
  FeSet(ctx, FeMakeSymbol(ctx, "stdout"), FeMakePtr(ctx, FexTFile, stdout));
  FeSet(ctx, FeMakeSymbol(ctx, "stderr"), FeMakePtr(ctx, FexTFile, stderr));
}

static FeObject* GetFile(FeContext* ctx, FeObject** arg) {
  FeObject* file = FeGetNextArgument(ctx, arg);
  if (FeGetType(file) != FexTFile) {
    FeHandleError(ctx, "not a file");
  }
  return file;
}

FeObject* FexCloseFile(FeContext* ctx, FeObject* arg) {
  if (fclose(FeToPtr(ctx, GetFile(ctx, &arg))) != 0) {
    FeHandleError(ctx, strerror(errno));
  }
  return &nil;
}

FeObject* FexOpenFile(FeContext* ctx, FeObject* arg) {
  char pathname[PATH_MAX + 1];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), pathname,
                   sizeof(pathname));
  char mode[8];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), mode, sizeof(mode));
  FILE* file = fopen(pathname, mode);
  if (file == NULL) {
    FeHandleError(ctx, strerror(errno));
  }
  return FeMakePtr(ctx, FexTFile, file);
}

FeObject* FexReadFile(FeContext* ctx, FeObject* arg) {
  FeObject* file = GetFile(ctx, &arg);
  char delimiter[16];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), delimiter,
                   sizeof(delimiter));

  char* record = NULL;
  size_t capacity = 0;
  // TODO: error checking!
  (void)getdelim(&record, &capacity, delimiter[0], FeToPtr(ctx, file));
  FeObject* result = FeMakeString(ctx, record);
  free(record);
  return result;
}

FeObject* FexWriteFile(FeContext* ctx, FeObject* arg) {
  FeObject* file = GetFile(ctx, &arg);
  const size_t arbitrary_limit = 4 * 1024 * 1024;  // TODO
  char* buffer = malloc(arbitrary_limit);
  const size_t size =
      FeToString(ctx, FeGetNextArgument(ctx, &arg), buffer, arbitrary_limit);
  // TODO: error checking!
  const size_t written = fwrite(buffer, 1, size, FeToPtr(ctx, file));
  free(buffer);
  return FeMakeDouble(ctx, (double)written);
}
