# Wolfram Elementary Cellular Automata

## Requirements

- [GLFW](https://www.glfw.org/)

## Quick Start

```
$ make
$ ./out/wolfram
```

## GLAD

This project uses [glad][] to load OpenGL functions. In the past, I have
supplied instruction for the user to generate these files locally due to some
complexity around the licensing of the files.

Does generating a file based on a specification count as a derivative work?

In any case, some file (or files) which were used to generate the following
files:

- vendor/glad/gl.h
- vendor/glad/gl.c

are licensed under Apache 2.0, so here's a notice just in case.

>    Copyright (c) 2013-2020 The Khronos Group Inc.
>
>    Licensed under the Apache License, Version 2.0 (the "License");
>    you may not use this file except in compliance with the License.
>    You may obtain a copy of the License at
>
>        http://www.apache.org/licenses/LICENSE-2.0
>
>    Unless required by applicable law or agreed to in writing, software
>    distributed under the License is distributed on an "AS IS" BASIS,
>    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
>    See the License for the specific language governing permissions and
>    limitations under the License.

### Modifications

These files to use local `#include` paths.

[glad]: <https://gen.glad.sh/>
