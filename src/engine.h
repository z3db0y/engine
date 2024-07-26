#pragma once
#include <vector>

#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace Engine
{
    static auto NAME = "";
    static constexpr int VERSION[3] = {0, 1, 0};

    class Game;

#ifdef _DEBUG
    static auto DEBUG = true;
#else
    static auto DEBUG = false;
#endif

#ifdef HAS_VULKAN
    static auto VK_ENABLE_VALIDATION_LAYERS = false;
    static std::vector VK_VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
    static std::vector VK_DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    class Renderer;
#endif
}

#include "core/game.h"
#include "core/renderer.h"
#include "loaders/loaders.h"