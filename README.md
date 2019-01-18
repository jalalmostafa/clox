# clox

[![Build Status](https://travis-ci.org/jalalmostafa/clox.svg?branch=master)](https://travis-ci.org/jalalmostafa/clox)
[![Build status](https://ci.appveyor.com/api/projects/status/xhuvod8r2je1juwd/branch/master?svg=true)](https://ci.appveyor.com/project/JalalMostafa/clox/branch/master)

clox is a cross-platform implementation of [lox](http://craftinginterpreters.com/the-lox-language.html) interpreter and virtual machine (soon!) in ANSI C.

Progress: [Ch.15 A Virtual Machine](http://craftinginterpreters.com/a-virtual-machine.html)

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

## License

clox can be freely distributed under the [MIT License](https://github.com/jalalmostafa/clox/blob/master/LICENSE)
