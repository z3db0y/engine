#include "core/renderer.h"

#include <iostream>

#ifndef HAS_VULKAN

int main()
{
    std::cerr << "couldn't find Vulkan" << '\n';
    return 1;
}

#elif !defined(HAS_GLFW)

int main()
{
    std::cerr << "couldn't find GLFW" << '\n';
    return 1;
}

#else

#include "engine.h"
#include <GLFW/glfw3.h>

using Engine::Game;
using Engine::Renderer;

int terminate(const Renderer* renderer, int code)
{
    delete renderer;
    glfwTerminate();

    return code;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "failed to initialize GLFW" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    uint32_t nExtensions;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&nExtensions);

    constexpr int version[] = {0, 1, 0};
    const auto game = new Game("Test", version);
    const auto renderer = new Renderer(game);

    const VkResult instResult = renderer->createInstance(nExtensions, extensions);

    if (instResult == VK_SUCCESS)
    {
        std::cout << "created instance successfully!" << '\n';
    } else
    {
        std::cerr << "failed to create renderer: " << instResult << '\n';
        return terminate(renderer, 1);
    }

    GLFWwindow* win = glfwCreateWindow(640, 480, "Test", nullptr, nullptr);

    if (win == nullptr)
    {
        std::cerr << "failed to create GLFWwindow" << '\n';
        return terminate(renderer, 1);
    }

    VkSurfaceKHR surface;
    const VkResult surfaceResult = glfwCreateWindowSurface(renderer->vkInst, win, nullptr, &surface);

    if (surfaceResult != VK_SUCCESS)
    {
        std::cerr << "failed to create window surface (" << surfaceResult << ')' << '\n';
        return terminate(renderer, 1);
    }

    const VkResult devResult = renderer->createDevice(surface);

    if (devResult == VK_SUCCESS)
    {
        std::cout << "created device successfully!" << '\n';
    } else
    {
        std::cerr << "failed to create device (" << devResult << ')' << '\n';
        return terminate(renderer, 1);
    }

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
    }

    return terminate(renderer, 0);
}

#endif