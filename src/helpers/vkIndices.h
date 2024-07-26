#pragma once
#include <cstdint>
#include <optional>
#include <set>

#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>

struct VkQueueIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool complete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    std::set<uint32_t> set()
    {
        return {graphicsFamily.value(), presentFamily.value()};
    }
};

VkQueueIndices vkQueueIndices(VkPhysicalDevice dev, VkSurfaceKHR surface);

#endif