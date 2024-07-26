#pragma once
#include <vector>

#ifdef HAS_VULKAN
#include "engine.h"
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

VkSwapChainDetails vkQuerySwapchain(VkPhysicalDevice dev, VkSurfaceKHR surface);
void vkSetupSwapchainCreateInfo(Engine::Renderer* renderer, VkSwapchainCreateInfoKHR& createInfo, VkSwapChainDetails& details);
void vkGetSwapchainImages(VkDevice dev, VkSwapchainKHR swapchain, std::vector<VkImage>* images);

#endif