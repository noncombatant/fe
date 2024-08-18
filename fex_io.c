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
}

FeObject* FexCloseFile(FeContext* ctx, FeObject* arg) {
  FeObject* file = FeGetNextArgument(ctx, &arg);
  const FeType type = FeGetType(file);
  if (type != FexTFile) {
    FeHandleError(ctx, "not a file");
  }
  if (fclose(FeToPtr(ctx, file)) != 0) {
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

ssize_t getdelim(char** restrict linep,
                 size_t* restrict linecapp,
                 int delimiter,
                 FILE* restrict stream);

FeObject* FexReadFile(FeContext* ctx, FeObject* arg) {
  FeObject* file = FeGetNextArgument(ctx, &arg);
  const FeType type = FeGetType(file);
  if (type != FexTFile) {
    FeHandleError(ctx, "not a file");
  }
  char delimiter[16];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), delimiter,
                   sizeof(delimiter));

  char* record = NULL;
  size_t capacity = 0;
  (void)getdelim(&record, &capacity, delimiter[0], FeToPtr(ctx, file));
  FeObject* result = FeMakeString(ctx, record);
  free(record);
  return result;
}
