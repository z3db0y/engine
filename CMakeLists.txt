cmake_minimum_required(VERSION 3.28.1)
project(engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
        src/engine.h
        src/core/renderer.h
        src/core/renderer.cpp
        src/core/game.cpp
        src/core/game.h
        src/helpers/vkIndices.h
        src/helpers/vkIndices.cpp
        src/helpers/vkDevice.h
        src/helpers/vkDevice.cpp
        src/helpers/vkVLayers.cpp
        src/helpers/vkVLayers.h
        src/helpers/vkSwapchain.h
        src/helpers/vkSwapchain.cpp
)

add_library(engine SHARED ${SOURCES})
target_include_directories(engine PUBLIC src)

if (EXISTS ${PROJECT_SOURCE_DIR}/libs/glfw)
    add_subdirectory(libs/glfw)
    target_link_libraries(engine PUBLIC glfw)

    add_compile_definitions(HAS_GLFW)
endif()

find_package(Vulkan QUIET)

if (Vulkan_FOUND)
    target_link_libraries(engine PUBLIC Vulkan::Vulkan)

    add_compile_definitions(HAS_VULKAN)
endif()

set(IS_DEBUG_BUILD CMAKE_BUILD_TYPE STREQUAL "Debug")

if (${IS_DEBUG_BUILD})
    add_compile_definitions(_DEBUG)
endif()

add_executable(test_renderer src/test/renderer.cpp)
target_link_libraries(test_renderer PRIVATE engine)