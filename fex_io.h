// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FEX_FILE_H
#define FEX_FILE_H

#include "fe.h"

void FexInstallIO(FeContext* ctx);

FeObject* FexOpen(FeContext* ctx, FeObject* arg);

#endif
