// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <stdlib.h>

#include "auto.h"
#include "fe.h"

void CloseFile(FILE** f) {
  if (*f == NULL) {
    return;
  } else if (fclose(*f) != 0) {
    perror("could not close file");
  }
}

void FreeChar(char** p) {
  free(*p);
}

void CloseContext(FeContext** c) {
  FeCloseContext(*c);
}
