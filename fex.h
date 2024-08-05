// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FEX_H
#define FEX_H

#include "fe.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn);
void FexInstallNativeFns(FeContext* ctx);

FeObject* FexAbs(FeContext* ctx, FeObject* arg);
FeObject* FexCeiling(FeContext* ctx, FeObject* arg);
FeObject* FexCubeRoot(FeContext* ctx, FeObject* arg);
FeObject* FexFloor(FeContext* ctx, FeObject* arg);
FeObject* FexHypotenuse(FeContext* ctx, FeObject* arg);
FeObject* FexIsFinite(FeContext* ctx, FeObject* arg);
FeObject* FexIsInfinite(FeContext* ctx, FeObject* arg);
FeObject* FexIsNaN(FeContext* ctx, FeObject* arg);
FeObject* FexIsNormal(FeContext* ctx, FeObject* arg);
FeObject* FexLg(FeContext* ctx, FeObject* arg);
FeObject* FexLog(FeContext* ctx, FeObject* arg);
FeObject* FexMax(FeContext* ctx, FeObject* arg);
FeObject* FexMin(FeContext* ctx, FeObject* arg);
FeObject* FexModulus(FeContext* ctx, FeObject* arg);
FeObject* FexNearbyInt(FeContext* ctx, FeObject* arg);
FeObject* FexPow(FeContext* ctx, FeObject* arg);
FeObject* FexRemainder(FeContext* ctx, FeObject* arg);
FeObject* FexRound(FeContext* ctx, FeObject* arg);
FeObject* FexRoundToInt(FeContext* ctx, FeObject* arg);
FeObject* FexSquareRoot(FeContext* ctx, FeObject* arg);
FeObject* FexTruncate(FeContext* ctx, FeObject* arg);

#endif
