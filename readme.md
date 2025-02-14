# Wolfram Elementary Cellular Automata

Generates a 640*480 image of an elementary cellular automata and saves a PNG.

- see: https://mathworld.wolfram.com/ElementaryCellularAutomaton.html

## Requirements

- Linux (`<getopt.h>`)
- [GLFW](https://www.glfw.org/)

## Quick Start

```
$ make
$ ./out/wolfram -r <rule>
```

## Generation modes

This program supports different "generation" modes, specified with the `-m`
flag.

### Standard

The standard mode (with the `-m` flag omitted) displays as usual.

```
$ ./out/wolfram -r 73
```

![](/assets/rule-73-standard.png)

### Directional

The "directional" mode colours each cell based on the parent cells state. For
example; If the left parent was in an activated state, then the "red" channel
of the new cell is turned on. Similarly for the middle and right parents, and
green and blue, respectively.

If the three parents being inactive sets the new cell to be active, it is
coloured in a dark grey.

```
$ ./out/wolfram -m directional -r 73
```

![](/assets/rule-73-directional.png)

### Split

The final mode "split" generates three images at once, one for each colour
channel. This can be used to show a difference between cellular automata. Here,
the green cells are present in rule 73, but not rule 1, and, conversely, the
magenta cells are present in rule 1, but not rule 73. White cells are present
in both rules 1 and 73.

```
$ ./out/wolfram -m split -r 1 -g 73 -b 1
```
![](/assets/rule-1-73-1-split.png)

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

## STB

The PNG is saved using [stb_image_write][]. Note: The file included within this
project is a cut down version of the original which only supports PNG.

[glad]: <https://gen.glad.sh/>
[stb_image_write]: <https://github.com/nothings/stb/blob/master/stb_image_write.h>
