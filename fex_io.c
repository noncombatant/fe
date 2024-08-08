// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "fex.h"
#include "fex_io.h"

void FexInstallIO(FeContext* ctx) {
  FexInstallNativeFn(ctx, "open", FexOpen);
}

FeObject* FexOpen(FeContext* ctx, FeObject* arg) {
  char pathname[PATH_MAX + 1];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), pathname,
                   sizeof(pathname));
  char mode[8];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), mode, sizeof(mode));
  FILE* file = fopen(pathname, mode);
  if (file == NULL) {
    return &nil;
  }
  return FeMakePtr(ctx, file);
  // TODO: Return a type-tagged object or a meaningful error object; define
  // these in fex.h. This will require a `FeToFex` as well, instead of Ptr.
}
