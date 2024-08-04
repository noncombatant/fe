# `fe`

`fe` is a tiny, embeddable, Lisp-like language implemented in ANSI C23.

## Overview

* Supports numbers, symbols, strings, pairs, lambdas, macros
* Lexically scoped variables, closures
* Small memory usage within a fixed-sized, caller-allocated arena — no `malloc`s
* Simple mark-and-sweep garbage collector
* Easy-to-use C API
* Portable ANSI C — works on 32- and 64-bit
* Concise — about 1,000 sloc

## Documentation

* [Example scripts](scripts) and their [test expectations](tests)
* [C API overview](doc/c-api.md)
* [Language overview](doc/language.md)
* [Implementation overview](doc/implementation.md)

## Contributing

Bug reports, pull requests, and questions are welcome.

## License

This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
