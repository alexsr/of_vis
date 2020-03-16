# Dependencies
The dependencies of this project are as follows:
* [Assimp](https://github.com/assimp/assimp) (http://www.assimp.org)

  Open Asset Import Library (short name: Assimp) is a portable Open Source library to import various well-known 3D model formats in a uniform manner.

* [Glad](https://github.com/Dav1dde/glad) (http://glad.dav1d.de)

  Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.

* [GLFW](https://github.com/glfw/glfw) (http://www.glfw.org/)

  GLFW is an Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan development on the desktop.
  It provides a simple API for creating windows, contexts and surfaces, receiving input and events.
  
* [GLM](https://github.com/g-truc/glm) (https://glm.g-truc.net/)

  OpenGL Mathematics (GLM) is a header only C++ mathematics library for graphics software based on the OpenGL Shading Language (GLSL) specifications.

* [Dear ImGui](https://github.com/ocornut/imgui) (http://glad.dav1d.de)

  Bloat-free Immediate Mode Graphical User interface for C++ with minimal dependencies.

I try to upgrade these as often as possible to always use the latest releases.
Therefore I don't provide version numbers here. If an upgrade breaks my code,
I will try to fix the issue as soon as possible.

## Dependencies structure

If you use the cmake modules provided by this project in the `cmake/` folder,
you should place the dependencies in the `external/` folder with the following structure:

```
external/
├── assimp/
│   ├── include/assimp/
│   └── lib/
├── glad/
│   ├── include/glad/
│   └── src/
├── glfw3/
│   ├── include/glfw/
│   └── lib/
├── glm/
│   └── glm/
├── imgui/
│   └──  imgui/
└── stb/
    └── stb/
```

### Dear ImGui specifics
For `FindImGUi.cmake` to work, either copy `imgui_impl_glfw.cpp` and `imgui_impl_opengl3.cpp`
to the `imgui/` folder or update the path to their source files to `imgui/examples/`.
I suggest you copy them, because otherwise you will most likely get linker errors in both `impl` files.