Declarations at first use throughout.

`const` as much as possible.

More example programs and tests.

Use `//` comments.

Replace `car` and `cdr` with `head` and `tail`, but keep `car`/`cdr` for
historical flair.

Unit test all fe functions.

When available, use `constexpr` instead of `#define`. Until then, use `enum`.

Use `readline` in main?

Reduce/eliminate C macros — replace the remaining lvalue accessors with `Set*`?

Finish adding the rest of math.h.

Add (at least) `fread`, `fwrite`, `fflush`, `fseeko`, `ftello`.

Add `regcomp` (as `(compile-regex ...)`) and `regexec` (as `(match-regex ...)`).

Consider namespaces, e.g. `math.*`, `io.*`, `sys.*`, et c.

Update docs given new names and types et c.

Turn the examples in the docs into scripts, and test them, and then refer to
them in the docs in place of the code.

Help strings for symbols.

Add GMP to Fex (or core Fe?) for better numbers.

Use bitfields instead of shifts and ors and so on.

Ensure that everything declared in fe.h really needs to be public.
