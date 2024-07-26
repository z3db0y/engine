#pragma once
#ifdef HAS_VULKAN

#include <vulkan/vulkan.h>

uint32_t deviceScore(VkPhysicalDevice dev, VkSurfaceKHR surface);
bool deviceSupportsExtensions(VkPhysicalDevice dev);

#endif