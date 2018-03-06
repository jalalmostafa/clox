# C-lox

[![Build Status](https://travis-ci.org/jalalmostafa/clox.svg?branch=master)](https://travis-ci.org/jalalmostafa/clox)
clox is a cross-platform implementation of [lox](http://craftinginterpreters.com/the-lox-language.html) treewalk interpreter in ANSI C.

Currently working on: [Statements and State](http://craftinginterpreters.com/statements-and-state.html)

## Coding Conventions

C-lox source code follows [Webkit Coding Convention](https://webkit.org/code-style-guidelines/). However, some rules are violated as follows:

* Function Naming: C-lox uses the old ugly but practical function naming e.g. `function_name()`
* Guards: C-lox uses guards instead of `#pragma once`
* Macro Naming: C-lox uses capital letters with underscores to name macros

## Build

In order to build and compile C-lox, you need to install `CMake` plus the toolchain of your choice, e.g. `MSBuild with MSVC` or `make with gcc`. After you setup [CMake](https://cmake.org/install) with your toolchain of choice, clone the repository and build the project as follows:

```bash
git clone https://github.com/jalalmostafa/clox.git
cd clox/
mkdir build && cd build
cmake ..
cmake --build .
```

In order to execute C-lox, check `/bin` folder in project directory for binaries.

Build tests have been conducted on:

* Windows x86_64 MSBuild and MSVC 19
* Windows x86_64 make and gcc 5.3
* Ubuntu x86_64 make and gcc 5.3

## Download

Download [lastest](https://github.com/jalalmostafa/clox/releases).

## License

C-Lox can be freely distributed under the [MIT License](https://github.com/jalalmostafa/c-lox/blob/master/LICENSE)
