# Engine

Ideally, this should be a game engine with 3D rendering using Vulkan, and with a physics engine inside as well.  
But right now, it's just me messing around :p

## Running tests

- Clone this repo
- Make a directory called `libs`
- Download GLFW and paste the GLFW folder into `libs` in such a way that `libs/glfw/CMakeLists.txt` exists.
- Use CMake to build any part of this project. (e.g. tests or engine itself - engine can be built without Vulkan/GLFW)