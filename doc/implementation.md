# Implementation

## Overview

The implementation aims to fulfill the following goals:

* Small memory usage within a fixed-sized memory region — no `malloc`s
* Practical for small scripts (extension scripts, config files)
* Concise source — about 1,000 sloc
* Portable ANSI C (Windows, POSIX, DOS; 32- and 64-bit)
* Simple and easy to understand source
* Simple and easy to use C API

The language offers the following:

* Numbers, symbols, strings, pairs, lambdas, macros, `cfunc`s, `ptr`s
* Lexically scoped variables
* Closures
* Variadic functions
* Mark-and-sweep garbage collector
* Stack traceback on error

## Memory

The implementation uses a fixed-sized region of memory supplied by the caller
when creating the `fe_Context`. The implementation stores the context at the
start of this memory region and uses the rest of the region to store
`fe_Object`s.

## Objects

All data is stored in fixed-sized `fe_Object`s. Each object consists of a `car`
and `cdr`. The lowest bit of an object’s `car` stores type information — if the
object is a `PAIR` (cons cell) the lowest bit is `0`, otherwise it is `1`. The
second-lowest bit is used by the garbage collector to mark the object and is
always `0` outside of the `collectgarbage` function.

Pairs use the `car` and `cdr` as pointers to other objects. As all objects are
at least 4 byte-aligned we can always assume the lower two bits on a pointer
referencing an object are `0`.

Non-pair objects store their full type in the first byte of `car`.

### String

Strings are stored using multiple objects of type `STRING` linked together —
each string object stores a part of the string in the bytes of `car` not used by
the type and GC mark. The `cdr` stores the object with the next part of the
string or `nil` if this was the last part of the string.

### Symbol

Symbols store a pair object in the `cdr`; the `car` of this pair contains a
`string` object, and the `cdr` part contains the globally bound value for the
symbol. Symbols are interned.

### Number

Numbers store a `Number` in the `cdr` part of the `object`. By default `Number`
is a `float`, but any value can be used so long as it is equal to or smaller in
size than an `fe_Object` pointer. If a different type of value is used,
`fe_read` and `fe_write` must also be updated to handle the new type correctly.

### Prim

Primitives (built-ins) store an `enum` in the `cdr` part of the object.

### CFunc

CFuncs store a `CFunc` pointer in the `cdr` part of the object.

### Ptr

`ptr`s store a `void` pointer in the `cdr` part of the object. The handler
functions `gc` and `mark` are called whenever a `ptr` is collected or marked by
the garbage collector — the set `fe_CFunc` is passed the object itself in place
of an arguments list.

## Environments

Environments are stored as association lists, for example: an environment with
the symbol `x` bound to `10` and `y` bound to `20` would be `((x . 10) (y .
20))`. Globally bound values are stored directly in the `symbol` object.

## Macros

Macros work similar to functions, but receive their arguments unevaluated and
return code which is evaluated in the scope of the caller. The first time a
macro is called the code which called it is replaced by the generated code, such
that the macro itself is only ran once in each place it is called. For example,
we could define the following macro to increment a value by one:

```clojure
(= incr
  (mac (sym)
    (list '= sym (list '+ sym 1))))
```

And use it in the following while loop:

```clojure
(= i 0)
(while (< i 0)
  (print i)
  (incr i))
```

Upon the first call to `incr`, the program code would be modified in-place,
replacing the call to the macro with the code it generated:

```clojure
(= i 0)
(while (< i 0)
  (print i)
  (= i (+ i 1)))
```

Subsequent iterations of the loop would run the new code which now exists where
the macro call was originally.

## Garbage Collection

`fe` uses a simple mark-and-sweep garbage collector in conjunction with a
freelist. When the context is initialized a freelist is created from all the
objects. When an object is required it is popped from the freelist. If there are
no more objects on the freelist the garbage collector does a full
mark-and-sweep, pushing unreachable objects back to the freelist, thus garbage
collection may occur whenever a new object is created.

The context maintains a `gcstack`, which protects objects that may not be
reachable from being collected. These may include, for example: objects returned
after an `eval`, or a list which is currently being constructed from multiple
pairs. Newly created objects are automatically pushed to this stack.

## Error Handling

If an error occurs `fe` calls `fe_error`. This function resets the context to a
safe state and calls the `error` handler if one is set. The error handler
function is passed the error message and a list representing the call stack.
(Both these values are valid only for this function’s duration.) The error
handler can be safely `longjmp`’d out of to recover from the error and use of
the context can continue — this can be seen in the REPL. Do not create new
objects from inside the error handler.

If no error handler is set or if the error handler returns, `fe` prints the
error message and call stack to `stderr` and calls `exit` is called with the
value `EXIT_FAILURE`.

## Known Issues

The implementation has some known issues; these exist as a side effect of trying
to keep the implementation terse, but should not hinder normal usage:

* The garbage collector recurses on the `car` of objects; thus, deeply nested
  `car`s may overflow the C stack — an object’s `cdr` is looped on and will not
  overflow the stack.
* The storage of an object’s type and GC mark assumes a little-endian system and
  will not work correctly on systems of other endianness.
* Proper tailcalls are not implemented — `while` can be used for iterating over
  lists.
* Strings are `NUL`-terminated and therefore not binary-safe.
