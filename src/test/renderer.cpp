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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

    constexpr int width = 640;
    constexpr int height = 480;

    GLFWwindow* win = glfwCreateWindow(width, height, "Test", nullptr, nullptr);

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

    const VkResult swapchainResult = renderer->createSwapchain(width, height);

    if (swapchainResult == VK_SUCCESS)
    {
        std::cout << "created swapchain successfully!" << '\n';
    } else
    {
        std::cerr << "failed to create swapchain (" << swapchainResult << ')' << '\n';
        return terminate(renderer, 1);
    }

    VkShaderModule vertShader;
    VkShaderModule fragShader;

    std::cout << "vertex shader result: " <<
        Loaders::loadShaderModule(renderer->vkDev, "../src/test/shaders/compiled/triangle.vert.spv", &vertShader) << '\n';

    std::cout << "fragment shader result: "  <<
        Loaders::loadShaderModule(renderer->vkDev, "../src/test/shaders/compiled/triangle.frag.spv", &fragShader) << '\n';

    VkPipelineShaderStageCreateInfo vertInfo{};
    vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertInfo.module = vertShader;
    vertInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragInfo{};
    fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragInfo.module = fragShader;
    fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragInfo.pName = "main";

    const VkResult pipelineResult = renderer->createRenderPipeline({vertInfo, fragInfo});

    if (pipelineResult == VK_SUCCESS)
    {
        std::cout << "created pipeline successfully!" << '\n';
    } else
    {
        std::cerr << "failed to create pipeline (" << pipelineResult << ')' << '\n';
        return terminate(renderer, 1);
    }

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        renderer->render();
    }

    return terminate(renderer, 0);
}

#endif