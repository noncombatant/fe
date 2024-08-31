// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <regex.h>
#include <stdlib.h>

#include "fex.h"
#include "fex_re.h"

void FexInstallRE(FeContext* ctx) {
  FexInstallNativeFn(ctx, "compile-re", FexCompileRE);
  FexInstallNativeFn(ctx, "match-re", FexMatchRE);

  // TODO: Any constants
}

enum {
  ArbitraryRELengthLimit = 4096,
  ArbitraryDataLengthLimit = 4 * 1024 * 1024,
};

static FeObject* BuildError(FeContext* ctx, int error, regex_t* re) {
  char message[1024];
  (void)regerror((error), (re), message, sizeof(message));
  return FeMakeList(
      ctx,
      (FeObject*[]){FeMakeDouble(ctx, (error)), FeMakeString((ctx), message)},
      2);
}

FeObject* FexCompileRE(FeContext* ctx, FeObject* arg) {
  char pattern[ArbitraryRELengthLimit + 1];
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), pattern, sizeof(pattern));

  regex_t* re = calloc(1, sizeof(regex_t));
  const int error = regcomp(re, pattern, REG_EXTENDED);
  return error == 0 ? FeMakePtr(ctx, FexTRE, re) : BuildError(ctx, error, re);
}

FeObject* FexMatchRE(FeContext* ctx, FeObject* arg) {
  FeObject* o = FeGetNextArgument(ctx, &arg);
  if (FeGetType(o) != FexTRE) {
    FeHandleError(ctx, "not a regular-expression");
  }
  regex_t* re = FeToPtr(ctx, o);

  const size_t buffer_size = 4 * 1024 * 1024;
  char* buffer = malloc(buffer_size);
  (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), buffer, buffer_size);

  regmatch_t matches[16];
  const int error = regexec(re, buffer, 16, matches, 0);
  free(buffer);
  return error == 0 ? FeMakeDouble(ctx, error) : BuildError(ctx, error, re);
}

FeObject* FexGCRE(FeContext* ctx, FeObject* o) {
  regex_t* re = FeToPtr(ctx, o);
  regfree(re);
  free(re);
  return &nil;
}
