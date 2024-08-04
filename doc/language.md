# Language

## Forms

### Special forms

#### `(let symbol value)`

Creates a new binding of `symbol` to the value `value` in the current environment.

#### `(= symbol value)`

Sets the existing binding of `symbol` to the value `value`; in lieu of an
existing binding the global value is set.

#### `(if condition then else ...)`

If `condition` is true evaluates `then`, else evaluates `else`. `else` and
`then` statements can be chained to replicate the functionality of `else if`
blocks.

```clojure
> (= x 2)
nil
> (if (is x 1) "one"
      (is x 2) "two"
      (is x 3) "three"
      "?")
two
```

#### `(fn arguments ...)`

Creates a new function.

```clojure
> (= sqr (fn (n) (* n n)))
nil
> (sqr 4)
16
```

#### `(mac arguments ...)`

Creates a new macro.

Macros work similar to functions, but receive their arguments unevaluated and
return code which is evaluated in the scope of the caller. The first time a
macro is called the call site is replaced by the generated code, such that the
macro itself is only run once in each call site.

For examples, see [macros.fe](../scripts/macros.fe).

#### `(while condition ...)`

If `condition` evaluates to true evaluates the rest of its arguments and keeps
repeating until `condition` evaluates to `nil`.

```clojure
> (= i 0)
nil
> (while (< i 3)
    (print i)
    (= i (+ i 1)))
0
1
2
nil
```

#### `(quote val)`

Returns `val` unevaluated.

```clojure
> (quote (hello world))
(hello world)
```

#### `(and ...)`

Evaluates each argument until one results in `nil` — the last argument’s value
is returned if all the arguments are true.

#### `(or ...)`

Evaluates each argument until one results in true, in which case that arguments
value is returned — `nil` is returned if no arguments are true.

#### `(do ...)`

Evaluates each of its arguments and returns the value of the last one.

### Functions

#### `(cons car cdr)`

Creates a new pair with the given `car` and `cdr` values.

#### `(car pair)`

Returns the `car` of the `pair` or `nil` if `pair` is `nil`.

#### `(cdr pair)`

Returns the `cdr` of the `pair` or `nil` if `pair` is `nil`.

#### `(setcar pair val)`

Sets the `car` of `pair` to `val`.

#### `(setcdr pair val)`

Sets the `cdr` of `pair` to `val`.

#### `(list ...)`

Returns all its arguments as a list.

```clojure
> (list 1 2 3)
(1 2 3)
```

#### `(not val)`

Returns true if `val` is `nil`, else returns `nil`.

```clojure
> (not 1)
nil
```

#### `(is a b)`

Returns true if the values `a` and `b` are equal in value. Numbers and strings
are equal if equivalent, all other values are equal only if they are the same
underlying object.

#### `(atom x)`

Returns true if `x` is not a pair, otherwise `nil`.

#### `(print ...)`

Prints all its arguments to `stdout`, each separated by a space and followed by
a newline.

#### `(< a b)`

Returns true if the numerical value `a` is less than `b`.

#### `(<= a b)`

Returns true if the numerical value `a` is less than or equal to `b`.

#### `(+ ...)`

Adds all its arguments together.

#### `(- ...)`

Subtracts all its arguments, left to right.

#### `(* ...)`

Multiplies all its arguments.

#### `(/ ...)`

Divides all its arguments, left to right.
