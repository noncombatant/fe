# Fe

Fe is a tiny, embeddable, Lisp-like language implemented in C23.

This version is a refactored and extended version of [the beautiful original by
rxi](https://github.com/rxi/fe).

## Overview

* Supports numbers, symbols, strings, pairs, lambdas, macros
* Lexically scoped variables, closures
* Small memory usage within a fixed-size, caller-allocated arena — no `malloc`s
* Simple mark-and-sweep garbage collector
* Easy-to-use C API
* Portable C — works on 32- and 64-bit
* Concise — the core language is about 1,100 source lines
* Extensible — a modular extension API, Fex, allows the core to remain stable

## Documentation

* [Example scripts](scripts) and their [test expectations](tests)
* [C API overview](doc/c-api.md)
* [Language overview](doc/language.md)
* [Implementation overview](doc/implementation.md)

## Contributing

Bug reports, pull requests, and questions are welcome.

## License

This program is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
