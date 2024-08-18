// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

#include "fe.h"
#include "fex.h"
#include "fex_io.h"
#include "fex_math.h"

static jmp_buf top_level;

static void noreturn HandleError(FeContext* ctx,
                                 const char* message,
                                 FeObject* stack) {
  fprintf(stderr, "error: %s\n", message);
  while (!FeIsNil(stack)) {
    char fn[1024];
    FeToString(ctx, FeCar(ctx, stack), fn, sizeof(fn));
    fprintf(stderr, "%s\n", fn);
    stack = FeCdr(ctx, stack);
  }
  longjmp(top_level, -1);
}

static void noreturn PrintHelp(int status) {
  // TODO
  exit(status);
}

int main(int count, char* arguments[]) {
  // Parse command line options:
  size_t arena_size = 64 * 1024;
  bool ignore_exceptions = false;
  while (true) {
    int ch = getopt(count, arguments, "his:");
    if (ch == -1) {
      break;
    }
    switch (ch) {
      case 'h':
        PrintHelp(EXIT_SUCCESS);
      case 'i':
        ignore_exceptions = true;
        break;
      case 's': {
        char* end = NULL;
        arena_size = strtoul(optarg, &end, 0);
        if (end == optarg) {
          PrintHelp(EXIT_FAILURE);
        }
        break;
      }
      default:
        PrintHelp(EXIT_FAILURE);
    }
  }
  count -= optind;
  arguments += optind;

  // Initialize the context:
  char* arena = malloc(arena_size);
  FeContext* ctx = FeOpenContext(arena, arena_size);
  FexInstallIO(ctx);
  FexInstallMath(ctx);

  FILE* input = stdin;
  if (count > 0) {
    input = fopen(arguments[0], "rb");
    if (!input) {
      FeHandleError(ctx, "could not open input file");
    }
  }

  if (input == stdin || ignore_exceptions) {
    FeGetHandlers(ctx)->error = HandleError;
  }
  size_t gc = FeSaveGC(ctx);
  setjmp(top_level);

  // REPL:
  while (true) {
    FeRestoreGC(ctx, gc);
    if (input == stdin) {
      printf("fe > ");
    }
    FeObject* obj = FeReadFile(ctx, input);
    if (obj == NULL) {
      break;
    }
    obj = FeEvaluate(ctx, obj);
    if (input == stdin) {
      FeWriteFile(ctx, obj, stdout);
      printf("\n");
    }
  }
  free(arena);
}
