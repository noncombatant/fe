// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
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
          "  fe [-i] [-s size] [program-file ...]\n\n"
          "Options:\n\n"
          "  -h    Print this help message and exit\n"
          "  -i    Interactive mode (read from stdin)\n"
          "  -s <size>\n"
          "        Set arena size\n");
  exit(status);
}

static size_t ReadEvaluatePrint(FeContext* context, FILE* input, size_t gc) {
  while (true) {
    FeRestoreGC(context, gc);
    if (input == stdin) {
      printf("fe > ");
    }
    FeObject* object = FeReadFile(context, input);
    if (object == NULL) {
      return FeSaveGC(context);
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
  bool interactive = false;
  bool program_literal = false;
  while (true) {
    int ch = getopt(count, arguments, "ehis:");
    if (ch == -1) {
      break;
    }
    switch (ch) {
      case 'e':
        program_literal = true;
        break;
      case 'h':
        PrintHelp(EXIT_SUCCESS);
      case 'i':
        interactive = true;
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
  if (interactive) {
    setjmp(top_level);
    FeGetHandlers(context)->error = HandleError;
  }

  // REPL the inputs:
  size_t gc = FeSaveGC(context);
  for (int i = 0; i < count; i++) {
    char* a = arguments[i];
    FILE* input =
        program_literal ? fmemopen(a, strlen(a), "rb") : fopen(a, "rb");
    if (!input) {
      FeHandleError(context, "could not open input file");
    }
    gc = ReadEvaluatePrint(context, input, gc);
    if (fclose(input)) {
      perror("could not close input");
    }
  }
  if (count == 0 || interactive) {
    setjmp(top_level);
    FeGetHandlers(context)->error = HandleError;
    ReadEvaluatePrint(context, stdin, gc);
  }

  free(arena);
}
