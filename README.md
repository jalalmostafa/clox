# C-lox

 C-lox is an implementation of [lox](http://craftinginterpreters.com/the-lox-language.html) interpreter in C.

## Coding Conventions

C-lox source code follows [Webkit Coding Convention](https://webkit.org/code-style-guidelines/). However, some rules are violated as follows:

* Function Naming: C-lox uses the old ugly but practical function naming e.g. `function_name()`
* Guards: C-lox uses guards instead of `#pragma once`
* Macro Naming: C-lox uses capital letters with underscores to name macros

## Build

| Platform | Build Dependencies |
| -------- | ------------------ |
| Windows  | [CMake](https://cmake.org/) and MVC++ 15 |
| Debian   | `sudo apt install cmake make` |

```bash
git clone https://github.com/jalalmostafa/c-lox.git
cd c-lox/
mkdir build && cd build
cmake ..
cmake --build .
```

Check `bin` for binaries.

## Download

Download [lastest](https://github.com/jalalmostafa/c-lox/releases).