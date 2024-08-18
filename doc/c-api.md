# C API

For the full details of the core Fe API, refer to `fe.h`. Here is an overview.

## Initializing A Context

To use Fe in your project, you must first initialize a `FeContext` by using the
`FeOpenContext` function. The function expects a block of memory (typically
greater than 16 KiB). Fe uses the block to store objects and context state, and
it must remain valid for the lifetime of the context. `FeCloseContext` should be
called when you are finished with a context. This assures that any `FePtr`
objects are properly garbage-collected.

```c
size_t size = 1024 * 1024;
void* data = malloc(size);
FeContext* ctx = FeOpenContext(data, size);

// ...

FeCloseContext(ctx);
free(data);
```

## Running A Script

To run a script, Fe must first read and then evaluate it. Do this in a loop if
there are several root-level expressions contained in the script. Fe provides
`FeReadFile` as a convenience to read from a file pointer, and you can also use
`FeRead` with a custom `FeReadFn` callback function to read from other sources.

```c
FILE* file = fopen("test.fe", "rb");
size_t gc = FeSaveGC(ctx);
while (true) {
  FeObject* obj = FeReadFile(ctx, file);
  // Break if there's nothing left to read.
  if (!obj) {
    break;
  }
  // Evaluate read object.
  FeEvalute(ctx, obj);

  // Restore GC stack which would now contain both the read object and
  // the result from evaluation.
  FeRestoreGC(ctx, gc);
}
fclose(file);
```

## Calling A Function

You can call a function by creating a list and evaulating it; for example, we
could add two numbers using the `+` function:

```c
size_t gc = FeSaveGC(ctx);

FeObject* objs[] = {
  FeMakeSymbol(ctx, "+"),
  FeMakeDouble(ctx, 10),
  FeMakeDouble(ctx, 20),
};
FeObject* result = FeEvaluate(ctx, FeMakeList(ctx, objs, 3));
printf("result: %g\n", FeToNumber(ctx, result));

// Discard all temporary objects pushed to the GC stack.
FeRestoreGC(ctx, gc);
```

## Extending The Core

For examples of using the extension API in full detail, refer to `fex.[ch]` and
`fex_*.[ch]`. Here is an overview.

### Exposing A C Function

You can install a `FeNativeFn` into a `FeContext` by using `FeMakeNativeFn`. The
`FeNativeFn` can be bound to a global variable by using the `FeSet` function.
`FeNativeFn`s take a context and an argument list as arguments, and return an
`FeObject`. The result must never be `NULL`; if you want to return `nil`, use
the value returned by `FeMakeBool(ctx, false)`.

You could expose the `pow` function from `math.h` like so:

```c
static FeObject* Power(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, pow(x, y));
}

FeSet(ctx, FeMakeSymbol(ctx, "pow"), FeMakeNativeFn(ctx, Power));
```

You can then call the `FeNativeFn` from Fe like any other function:

```clojure
(print (pow 2 10))
```

### Creating An `FePtr`

Fe provides the `FePtr` object type to allow for custom objects. For type
safety, you must tag your types by using the `FeTFex*` elements of the `FeType`
`enum` type. You can give them your own `enum` names:

```c
enum {
  MyTypeFoo = FeTFex0,
  MyTypeBar = FeTFex1,
  // ...
};
```

And you should give them string names, too, using the global `type_names` array:

```c
type_names[MyTypeFoo] = "foo";
type_names[MyTypeBar] = "bar";
```

You can create an `FePtr` object by using the `FeMakePtr` function.

Fe provides the `gc` and `mark` `FeHandler`s to customize garbage collection for
your `FePtr` types. Whenever the GC marks an `FePtr`, it calls the `mark`
handler on it — this is useful if the `FePtr` stores additional objects which
also need to be marked via `FeMark`. Fe calls the `gc` handler on the `FePtr`
when it becomes unreachable and collects it, such that the resources used by the
`FePtr` can be freed. You can set the handlers by setting the relevant fields in
the `FeHandlers` returned by `FeGetHandlers`.

### Error Handling

When an error occurs, Fe calls `FeHandleError`. By default, Fe prints the error
and stack trace and the program exits. If you want to recover from an error, set
the `error` handler field in the `FeHandlers` returned by `FeGetHandlers` and
use `longjmp` to exit the handler. Fe leaves the context in a safe state and you
can continue to use it. However, do not create new `FeObject`s inside the error
handler.
