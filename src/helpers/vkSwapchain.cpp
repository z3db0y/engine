#include "vkSwapchain.h"
#ifdef HAS_VULKAN

VkSwapChainDetails querySwapChain(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    VkSwapChainDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

    uint32_t nFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &nFormats, nullptr);

    if (nFormats > 0)
    {
        details.formats.resize(nFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &nFormats, details.formats.data());
    }

    uint32_t nPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &nPresentModes, nullptr);

    if (nPresentModes > 0)
    {
        details.presentModes.resize(nPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &nPresentModes, details.presentModes.data());
    }

    return details;
}


#endif