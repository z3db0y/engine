#include "vkDevice.h"

#include "vkSwapchain.h"
#ifdef HAS_VULKAN

#include <iostream>
#include <cstring>
#include <vector>

#include "engine.h"
#include "vkIndices.h"

uint32_t deviceScore(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(dev, &props);

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += props.limits.maxImageDimension3D;

    VkQueueIndices indices = vkQueueIndices(dev, surface);
    VkSwapChainDetails swapChain = querySwapChain(dev, surface);

    bool extensionSupport = deviceSupportsExtensions(dev);

    if (!indices.complete() || !extensionSupport || !swapChain.supported())
    {
        return 0;
    }

    return score;
}

bool deviceSupportsExtensions(VkPhysicalDevice dev)
{
    uint32_t nExtensions;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &nExtensions, nullptr);

    std::vector<VkExtensionProperties> extensions(nExtensions);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &nExtensions, extensions.data());

    for (const auto& requested : Engine::VK_DEVICE_EXTENSIONS)
    {
        bool found = false;

        for (const auto& ext : extensions)
        {
            found = strcmp(ext.extensionName, requested);
            if (found) break;
        }

        if (!found) return false;
    }

    return true;
}

#endif