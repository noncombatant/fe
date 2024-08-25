// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fex.h"
#include "fex_process.h"

void FexInstallProcess(FeContext* ctx) {
  FexInstallNativeFn(ctx, "execute", FexExecute);
}

FeObject* FexExecute(FeContext* ctx, FeObject* arg) {
  enum { MaxArgumentCount = 31 };
  char* arguments[MaxArgumentCount + 1] = {NULL};
  size_t i;
  for (i = 0; i < MaxArgumentCount; i++) {
    if (FeIsNil(arg)) {
      break;
    }
    FeObject* a = FeGetNextArgument(ctx, &arg);
    const FeType type = FeGetType(a);
    if (type != FeTString) {
      FeHandleError(ctx, "not a string");
    }
    char string[1024];
    FeToString(ctx, a, string, sizeof(string));
    arguments[i] = strdup(string);
  }

  if (i == 0) {
    FeHandleError(ctx, "not enough arguments");
  }

  int status = -1;
  const pid_t child = fork();
  if (child == 0) {
    execvp(arguments[0], arguments);
  } else if (child == -1) {
    perror("fork");
  } else {
    if (waitpid(child, &status, 0) == -1) {
      perror("waitpid");
    } else {
      status = WEXITSTATUS(status);
    }
  }

  for (i = 0; i < MaxArgumentCount && arguments[i] != NULL; i++) {
    free(arguments[i]);
  }
  return FeMakeDouble(ctx, (double)status);
}
