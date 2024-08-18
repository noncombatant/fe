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
  FILE* out = status == 0 ? stdout : stderr;
  fprintf(out,
          "fe — Fe language interpreter\n\n"
          "Usage:\n\n"
          "  fe -h\n"
          "  fe [-i] [-s size] [program-file]\n\n"
          "Options:\n\n"
          "  -h    Print this help message and exit\n"
          "  -i    Continue from exceptions\n"
          "  -s <size>\n"
          "        Set arena size\n");
  exit(status);
}

static void ReadEvaluatePrint(FeContext* context, FILE* input) {
  size_t gc = FeSaveGC(context);
  while (true) {
    FeRestoreGC(context, gc);
    if (input == stdin) {
      printf("fe > ");
    }
    FeObject* object = FeReadFile(context, input);
    if (object == NULL) {
      break;
    }
    object = FeEvaluate(context, object);
    if (input == stdin) {
      FeWriteFile(context, object, stdout);
      printf("\n");
    }
  }
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
  FeContext* context = FeOpenContext(arena, arena_size);
  FexInstallIO(context);
  FexInstallMath(context);

  FILE* input = count > 0 ? fopen(arguments[0], "rb") : stdin;
  if (!input) {
    FeHandleError(context, "could not open input file");
  }

  if (input == stdin || ignore_exceptions) {
    setjmp(top_level);
    FeGetHandlers(context)->error = HandleError;
  }

  ReadEvaluatePrint(context, input);

  free(arena);
}
