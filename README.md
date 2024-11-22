# Prosper
Rendering a triangle using [Vulkan](https://www.vulkan.org/), the [Slang](https://shader-slang.com/) shader language, and [SDL3](https://wiki.libsdl.org/SDL3/FrontPage).

![prosper](https://github.com/user-attachments/assets/b6c25ff8-4819-4e0a-a845-8ec5e1df4333)

Mostly based on the first chapters of [vulkan-tutorial.com](https://vulkan-tutorial.com/), with the main difference being that I use SDL3 instead of glfw and Slang instead of GLSL. Also very helpful resources were the [Vulkan Lecture Series](https://www.youtube.com/playlist?list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgn) by TU Wien and this [introduction to SDL3](https://lazyfoo.net/tutorials/SDL3/01-hello-sdl3/index2.php) by Lazy Foo' Productions.

This is just a proof of concept example that hard-codes everything. The code may need adjustment to run on your GPU because I skipped some checks that one should do to select the right device and graphics queue.

## Installation instructions
### Linux (and probably Mac and BSD)
You'll need to have installed the Vulkan SDK and SDL3 libraries on your system. To build with CMake run these commands in the terminal inside the project's directoy:

```
cmake .
cmake --build .
```

The executable then gets created in `build/bin` if everything went right. 
Regarding the shader, it is compiled to bytecode and included with `shaders/bin/triangle.h` and its source code is in `shaders/src/triangle.slang`. If you want to modify it you'll have to compile it with [`slangc`](https://github.com/shader-slang/slang). The `shaders` directory contains a bash script with the compile commands I used, you will need to adjust path to the slangc binary to point to where it is installed on your system.

### Windows
idk, you're on your own ¯\_(ツ)_/¯

## License
All source files are licensed under CC0.
