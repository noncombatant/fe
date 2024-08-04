# C API

## Initializing a context

To use `fe` in your project, you must first initialize a `fe_Context` by using
the `fe_open` function. The function expects a block of memory (typically
greater than 16 KiB). `fe` uses the block to store objects and context state,
and it must remain valid for the lifetime of the context. `fe_close` should be
called when you are finished with a context. This assures that any `ptr` objects
are properly garbage-collected.

```c
size_t size = 1024 * 1024;
void* data = malloc(size);

fe_Context* ctx = fe_open(data, size);

// ...

fe_close(ctx);
free(data);
```

## Running a script

To run a script `fe` must first read and then evaluate it. Do this in a loop if
there are several root-level expressions contained in the script. `fe` provides
`fe_readfp` as a convenience to read from a file pointer, and you can also use
`fe_read` with a custom `fe_ReadFn` callback function to read from other
sources.

```c
FILE* fp = fopen("test.fe", "rb");
size_t gc = fe_savegc(ctx);
while (true) {
  fe_Object* obj = fe_readfp(ctx, fp);

  /* break if there's nothing left to read */
  if (!obj) {
    break;
  }

  /* evaluate read object */
  fe_eval(ctx, obj);

  /* restore GC stack which would now contain both the read object and
  ** result from evaluation */
  fe_restoregc(ctx, gc);
}
fclose(fp);
```

## Calling a function

You can call a function by creating a list and evaulating it; for example, we
could add two numbers using the `+` function:

```c
size_t gc = fe_savegc(ctx);

fe_Object* objs[3];
objs[0] = fe_symbol(ctx, "+");
objs[1] = fe_number(ctx, 10);
objs[2] = fe_number(ctx, 20);

fe_Object* result = fe_eval(ctx, fe_list(ctx, objs, 3));
printf("result: %g\n", fe_tonumber(ctx, result));

/* discard all temporary objects pushed to the GC stack */
fe_restoregc(ctx, gc);
```

## Exposing a C function

You can create a `cfunc` by using the `fe_cfunc` function with a `fe_CFunc`
function argument. The `cfunc` can be bound to a global variable by using the
`fe_set` function. `cfunc`s take a context and argument list as its arguments
and returns a result object. The result must never be `NULL`; if you want to
return `nil`, use the value returned by `fe_bool(ctx, false)`.

You could expose the `pow` function from `math.h` like so:

```c
static fe_Object* f_pow(fe_Context* ctx, fe_Object* arg) {
  float x = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float y = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  return fe_number(ctx, pow(x, y));
}

fe_set(ctx, fe_symbol(ctx, "pow"), fe_cfunc(ctx, f_pow));
```

You can then call the `cfunc` like any other function:

```clojure
(print (pow 2 10))
```

## Creating a `ptr`

`fe` provides the `ptr` object type to allow for custom objects. `fe` performs
no type checking on custom types, and thus you must wrap and tag pointers to
assure type safety if you use more than one type.

You can create a `ptr` object by using the `fe_ptr` function.

`fe` provides the `gc` and `mark` handlers for dealing with `ptr`s regarding
garbage collection. Whenever the GC marks a `ptr`, it calls the `mark` handler
on it — this is useful if the `ptr` stores additional objects which also need to
be marked via `fe_mark`. `fe` calls the `gc` handler on the `ptr` when it
becomes unreachable and garbage-collects it, such that the resources used by the
`ptr` can be freed. You can set the handlers by setting the relevant fields in
the struct returned by `fe_handlers`.

## Error handling

When an error occurs, `fe` calls `fe_error`. By default, `fe` prints the error
and stack trace and the program exits. If you want to recover from an error, set
the `error` handler field in the struct returned by `fe_handlers` and use
`longjmp` to exit the handler. `fe` leaves the context in a safe state and you
can continue to use it. Do not create New `fe_Object`s  inside the error
handler.
