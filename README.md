# C-lox

 C-lox is an implementation of [lox](http://craftinginterpreters.com/the-lox-language.html) interpreter in C.

## Coding Conventions

C-lox source code follows [Webkit Coding Convention](https://webkit.org/code-style-guidelines/). However, some rules are violated as follows:

* Function Naming: C-lox uses the old ugly but practical function naming e.g. `function_name()`
* Guards: C-lox uses guards instead of `#pragma once`
* Macro Naming: C-lox uses capital letters with underscores to name macros

## Build

In order to build and compile C-lox, you need to install `CMake` plus the toolchain of your choice, e.g. `MSBuild with MSVC++` or `make with gcc`

| Platform | Build Dependencies |
| -------- | ------------------ |
| Windows  | [CMake](https://cmake.org/) |

After you setup your `CMake` with your toolchain of choice, clone the repository and build the project as follows:

```bash
git clone https://github.com/jalalmostafa/c-lox.git
cd c-lox/
mkdir build && cd build
cmake ..
cmake --build .
```

In order to execute C-lox, check `/bin` folder in project directory for binaries.

Build tests have been conducted as follows:

* Windows x86_64 MSBuild and MSVC++ 15
* Windows x86_64 make and gcc

## Download

Download [lastest](https://github.com/jalalmostafa/c-lox/releases).

## License

C-Lox can be freely distributed under the [MIT License](https://github.com/jalalmostafa/c-lox/blob/master/LICENSE)
