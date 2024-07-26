#pragma once
#include <vector>

#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>

struct VkSwapChainDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    [[nodiscard]] bool supported() const
    {
        return !this->formats.empty() && !this->presentModes.empty();
    }
};

VkSwapChainDetails querySwapChain(VkPhysicalDevice dev, VkSurfaceKHR surface);

#endif