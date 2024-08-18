// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "fex.h"
#include "fex_io.h"

void FexInstallIO(FeContext* ctx) {
  FexInstallNativeFn(ctx, "close-file", FexCloseFile);
  FexInstallNativeFn(ctx, "open-file", FexOpenFile);
}

FeObject* FexOpenFile(FeContext* ctx, FeObject* arg) {
  char pathname[PATH_MAX + 1];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), pathname,
                   sizeof(pathname));
  char mode[8];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), mode, sizeof(mode));
  FILE* file = fopen(pathname, mode);
  return file != NULL ? FeMakePtr(ctx, FexTFile, file)
                      : FeMakePtr(ctx, FexTError, (void*)(uintptr_t)errno);
}

FeObject* FexCloseFile(FeContext* ctx, FeObject* arg) {
  FeObject* file = FeGetNextArgument(ctx, &arg);
  const FeType type = FeGetType(file);
  if (type != FexTFile) {
    return FeMakePtr(ctx, FexTError, (void*)(uintptr_t)type);
  }
  return fclose(FeToPtr(ctx, file)) == 0
             ? &nil
             : FeMakePtr(ctx, FexTError, (void*)(uintptr_t)errno);
}
