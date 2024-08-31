Declarations at first use throughout.

`const` as much as possible.

More example programs and tests.

Replace `car` and `cdr` with `head` and `tail`, but keep `car`/`cdr` for
historical flair.

Unit test all fe functions.

When available, use `constexpr` instead of `enum`.

Use `readline` in main?

Reduce/eliminate C macros — replace the remaining lvalue accessors with `Set*`?

Use GMP for numbers, and expose all of libgmp.

Add (at least) `fflush`, `fseeko`, `ftello`, `remove-file` (`remove`).

Add `regcomp` (as `(compile-regex ...)`) and `regexec` (as `(match-regex ...)`).

Consider namespaces, e.g. `math.*`, `io.*`, `sys.*`, et c.

Update docs given new names and types et c.

Turn the examples in the docs into scripts, and test them, and then refer to
them in the docs in place of the code.

Help strings for symbols.

Use bitfields instead of shifts and ors and so on.

Ensure that everything declared in fe.h really needs to be public.
