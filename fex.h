// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include "fe.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn);
void FexInstallNativeFns(FeContext* ctx);

FeObject* FexAbs(FeContext* ctx, FeObject* arg);
FeObject* FexCeiling(FeContext* ctx, FeObject* arg);
FeObject* FexFloor(FeContext* ctx, FeObject* arg);
FeObject* FexIsFinite(FeContext* ctx, FeObject* arg);
FeObject* FexIsInfinite(FeContext* ctx, FeObject* arg);
FeObject* FexIsNaN(FeContext* ctx, FeObject* arg);
FeObject* FexIsNormal(FeContext* ctx, FeObject* arg);
FeObject* FexMax(FeContext* ctx, FeObject* arg);
FeObject* FexMin(FeContext* ctx, FeObject* arg);
FeObject* FexNearbyInt(FeContext* ctx, FeObject* arg);
FeObject* FexPow(FeContext* ctx, FeObject* arg);
FeObject* FexRound(FeContext* ctx, FeObject* arg);
FeObject* FexRoundToInt(FeContext* ctx, FeObject* arg);
FeObject* FexTruncate(FeContext* ctx, FeObject* arg);
