// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <sys/time.h>

#include <errno.h>

#include "fex.h"
#include "fex_time.h"

void FexInstallTime(FeContext* ctx) {
  FexInstallNativeFn(ctx, "get-time", FexGetTime);
}

FeObject* FexGetTime(FeContext* ctx, FeObject*) {
  struct timeval time;
  struct timezone zone;
  const int r = gettimeofday(&time, &zone);
  if (r != 0) {
    return BuildErrnoError(ctx, errno);
  }
  const double seconds =
      (double)time.tv_sec + (((double)time.tv_usec) / 1000000.0);
  fprintf(stderr, "%f %g\n", seconds, seconds);
  return FeMakeList(
      ctx,
      (FeObject*[]){FeMakeDouble(ctx, (double)time.tv_sec),
                    FeMakeDouble(ctx, (double)time.tv_usec),
                    FeMakeDouble(ctx, (double)zone.tz_minuteswest),
                    FeMakeDouble(ctx, (double)zone.tz_dsttime)},
      4);
}
