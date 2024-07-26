#include "vkIndices.h"
#include <vector>

#ifdef HAS_VULKAN

VkQueueIndices vkQueueIndices(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    VkQueueIndices indices{};

    uint32_t nQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &nQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(nQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &nQueueFamilies, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.complete()) break;
        i++;
    }

    return indices;
}

#endif