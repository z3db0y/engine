#pragma once
#include "engine.h"

#ifdef HAS_VULKAN

class Engine::Renderer
{
    Game* game;

    VkDevice vkDev{};
    VkQueue vkDevQueue{};
    VkQueue vkPresentQueue{};
public:
    VkInstance vkInst{};

    explicit Renderer(Game* game);
    VkResult createInstance(uint32_t vkExtensionCount, const char* const* vkExtensionNames);
    VkResult createDevice(VkSurfaceKHR surface);

    ~Renderer();
};

#endif