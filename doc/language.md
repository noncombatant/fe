# Language

## Forms

### Special Forms

#### `(let symbol value)`

Creates a new binding of `symbol` to the value `value` in the current environment.

#### `(= symbol value)`

Sets the existing binding of `symbol` to the value `value` in the current
environment. If there is no such binding in the current environment, creates or
updates a binding in the global environment.

#### `(if condition then else ...)`

If `condition` is true, evaluates `then`; otherwise, evaluates `else`.

```clojure
fe > (= x 2)
nil
fe > (if (is x 1) "one"
         "bleep")
bleep
```

`else` and `then` expressions can be chained to replicate the functionality of
C’s `else if` and `switch`/`case` statements:

```clojure
fe > (if (is x 1) "one"
         (is x 2) "two"
         (is x 3) "three"
         "?")
two
```

#### `(fn arguments ...)`

Creates a new function.

```clojure
fe > (= square (fn (n) (* n n)))
nil
fe > (square 4)
16
```

#### `(macro arguments ...)`

Creates a new macro.

Macros work similar to functions, but receive their arguments unevaluated and
return code which is evaluated in the scope of the caller. The first time a
macro is called the call site is replaced by the generated code, such that the
macro itself is only run once in each call site.

For example, we could define a macro named `++` to increment a numeric value by
1:

```clojure
(= ++
  (macro (sym)
  (list '= sym (list '+ sym 1))))
```

Then we could use it in the following `while` loop:

```clojure
(= i 0)
(while (< i 10)
  (print i)
  (++ i))
```

Upon the first call to `++`, the program code would be modified in place,
replacing the call to the macro with the code it generated. Thus, the above code
expands to, and is equivalent to, this code:

```clojure
(= i 0)
(while (< i 0)
  (print i)
  (= i (+ i 1)))
```

Subsequent iterations of the loop would run the new code which now exists where
the macro call was originally.

For more examples, see [macros.fe](../scripts/macros.fe).

#### `(while condition ...)`

If `condition` evaluates to true, evaluates the rest of its arguments. Repeats
this process until `condition` evaluates to `nil`.

```clojure
fe > (= i 0)
nil
fe > (while (< i 3)
       (print i)
       (= i (+ i 1)))
0
1
2
nil
```

#### `(quote expression)`

Returns `expression` unevaluated.

```clojure
fe > wow
nil  ;; There is no binding for the name wow, so its value is nil.
fe > (quote wow)
wow
fe > (hello world)
error: tried to call non-callable value  ;; There is no binding for the name
                                         ;; hello, so its value is nil. And,
                                         ;; nil is not callable.
fe > (quote (hello world))
(hello world)
```

As a bit of syntactic sugar, you can also use the single quote, `'expression`,
without parentheses:

```clojure
fe > 'wow
wow
fe > '(hello world)
(hello world)
```

#### `(and ...)`

Evaluates each argument until one results in `nil` — the last argument’s value
is returned if all the arguments are true.

```clojure
fe > (and 1 2 3)
3
fe > (and 1 nil 3)
nil
```

#### `(or ...)`

Evaluates each argument until one results in true, in which case that argument’s
value is returned. Returns `nil` if no arguments are true.

```clojure
fe > (or 1 2 3)
1
fe > (or nil 2 3)
2
```

#### `(do ...)`

Evaluates each of its arguments and returns the value of the last one.

```clojure
fe > (do
       (print "wow")
       nil
       (print 2 nil)
       (+ 3 4))
wow
2 nil
7
```

### Functions

#### `(cons car cdr)`

Creates a new pair with the given `car` and `cdr` values.

```clojure
fe > (= p (cons 1 2))
nil
fe > p
(1 . 2)
```

#### `(car pair)`

Returns the first element of the `pair`, or `nil` if `pair` is `nil`.

```clojure
fe > p
(3 . 4)
fe > (car p)
3
```

#### `(cdr pair)`

Returns the second element of the `pair`, or `nil` if `pair` is `nil`.

```clojure
fe > p
(3 . 4)
fe > (car p)
3
fe > (cdr p)
4
```

#### `(env)`

Returns a list of all symbols in the current environment.

#### `(setcar pair value)`

Sets the first element of `pair` to `value`.

```clojure
fe > (= p (cons 1 2))
nil
fe > p
(1 . 2)
fe > (setcar p 3)
nil
fe > p
(3 . 2)
```

#### `(setcdr pair value)`

Sets the second element of `pair` to `value`.

```clojure
fe > (= p (cons 1 2))
nil
fe > p
(1 . 2)
fe > (setcdr p 4)
nil
fe > p
(1 . 4)
```

#### `(list ...)`

Returns all its arguments as a list.

```clojure
fe > (list 1 2 3)
(1 2 3)
```

#### `(not value)`

Returns true if `value` is `nil`, else returns `nil`.

```clojure
fe > (not 1)
nil
fe > (not nil)
t
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
