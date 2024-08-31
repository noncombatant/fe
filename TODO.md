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

Consider namespaces, e.g. `math.*`, `io.*`, `sys.*`, et c.

Update docs given new names and types et c.

Turn the examples in the docs into scripts, and test them, and then refer to
them in the docs in place of the code.

Help strings for symbols. E.g. when evaluating a function, if the first element
is a string, just ignore it (so that consing it doesn't waste resources).

Use bitfields for the "is cons cell" and GC mark bits, instead of shifts and ors
and so on.

Ensure that everything declared in fe.h really needs to be public.

Functions add their arguments to the global environment, but they should be
creating their own and destorying it upon return. Similarly, just naming a
non-existent variable creates it in the global env, but should not.

Handle errors in fex_io the same as in fex_re: return lists that describe the
object.
