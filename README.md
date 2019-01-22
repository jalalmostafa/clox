# CLox

[![Build Status](https://travis-ci.org/jalalmostafa/clox.svg?branch=master)](https://travis-ci.org/jalalmostafa/clox)
[![Build status](https://ci.appveyor.com/api/projects/status/xhuvod8r2je1juwd/branch/master?svg=true)](https://ci.appveyor.com/project/JalalMostafa/clox/branch/master)

CLox is a cross-platform implementation of a tree-walk [lox](http://craftinginterpreters.com/the-lox-language.html) interpreter and a virtual machine (soon!) in C89. Both modes use the same tokenizer that is available in `include/tokenizer.h` and `src/tokenizer.c`.

Progress: [Ch.18 Types of Values](http://craftinginterpreters.com/types-of-values.html)

## How to Run

```bash
`lox.exe <filename>` or just `lox.exe` to launch REPL interpreter.
    --tree-walk    runs clox in tree walk mode
    --vm           runs clox in bytecode mode (default)
    --help         shows this help text
```

## Coding Conventions

clox source code follows [Webkit Coding Convention](https://webkit.org/code-style-guidelines/). However, some rules are violated as follows:

* Function Naming: clox uses the old ugly but practical function naming e.g. `function_name()`
* Guards: clox uses guards instead of `#pragma once`
* Macro Naming: clox uses capital letters with underscores to name macros

## Build

In order to build and compile clox, you need to install `CMake` plus the toolchain of your choice, e.g. `MSBuild with MSVC` or `make with gcc`. After you setup [CMake](https://cmake.org/install) with your toolchain of choice, clone the repository and build the project as follows:

```bash
git clone https://github.com/jalalmostafa/clox.git
cd clox/
mkdir build && cd build
cmake ..
cmake --build .
```

In order to execute clox, check `bin` folder in project directory for binaries. Execute with `--tree-walk` in the arguments.

### VS Code

You can also use `VS Code` to automate builds and run tests. Below is a table of the available tasks. If it is the first time running the project, then be sure to generate build files by running `Rebuild` task

| Task Name                   | Job                                                                               | Hotkey (default) |
| --------------------------- | --------------------------------------------------------------------------------- |:----------------:|
| Build                       | Compile source code and generate binaries                                         | `Ctrl+Shift+B`   |
| Rebuild                     | Run `cmake` to generate build files then run `Build` task                         |                  |
| Run Read Test with Treewalk | Run `clox` with `examples/read_from_input.lox` and `--tree-walk` in the arguments |                  |
| Run Read Test with VM       | Run `clox` with `examples/read_from_input.lox` and `--vm` in the arguments        |                  |
