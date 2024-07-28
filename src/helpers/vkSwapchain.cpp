#include "vkSwapchain.h"

#include <bits/stl_algo.h>
#ifdef HAS_VULKAN

VkSwapChainDetails vkQuerySwapchain(VkPhysicalDevice dev, VkSurfaceKHR surface)
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

void vkSetupSwapchainCreateInfo(Engine::Renderer* renderer, VkSwapchainCreateInfoKHR& createInfo, VkSwapChainDetails& details)
{
    createInfo.imageExtent.width = std::clamp(
        createInfo.imageExtent.width,
        details.capabilities.minImageExtent.width,
        details.capabilities.maxImageExtent.width);

    createInfo.imageExtent.height = std::clamp(
        createInfo.imageExtent.height,
        details.capabilities.minImageExtent.height,
        details.capabilities.maxImageExtent.height);

    VkSurfaceFormatKHR surfaceFormat = details.formats[0];
    VkPresentModeKHR presentMode = details.presentModes[0];

    for (const auto& format : details.formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = format;
        }
    }

    for (const auto& mode : details.presentModes)
    {
        if (mode == renderer->preferredPresentMode)
        {
            presentMode = mode;
            break;
        }
    }

    uint32_t imageCount = details.capabilities.minImageCount + 1;

    if (imageCount > details.capabilities.maxImageCount && details.capabilities.maxImageCount > 0)
    {
        imageCount = details.capabilities.maxImageCount;
    }

    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.presentMode = presentMode;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.minImageCount = imageCount;
}

void vkGetSwapchainImages(VkDevice dev, VkSwapchainKHR swapchain, std::vector<VkImage>* images)
{
    uint32_t nImages;
    vkGetSwapchainImagesKHR(dev, swapchain, &nImages, nullptr);

    images->resize(nImages);
    vkGetSwapchainImagesKHR(dev, swapchain, &nImages, images->data());
}

#endif