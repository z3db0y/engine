#include "renderer.h"

#ifdef HAS_VULKAN

#include <cstring>
#include <map>
#include <vector>
#include <bits/stl_algo.h>

#include "helpers/vkSwapchain.h"
#include "helpers/vkVLayers.h"
#include "helpers/vkDevice.h"
#include "helpers/vkIndices.h"
#include "game.h"

VkResult tryMacFix(
    VkInstanceCreateInfo vkCreateInfo,
    VkInstance* pInst
) {
    std::vector<const char*> extensions;

    for (uint32_t i = 0; i < vkCreateInfo.enabledExtensionCount; i++)
    {
        const char* name = vkCreateInfo.ppEnabledExtensionNames[i];

        if (strcmp(name, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
        {
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }

        extensions.emplace_back(name);
    }

    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    vkCreateInfo.ppEnabledExtensionNames = extensions.data();

    vkCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    return vkCreateInstance(&vkCreateInfo, nullptr, pInst);
}

Engine::Renderer::Renderer(Game* game)
{
    this->game = game;
}

VkResult Engine::Renderer::createInstance(
    const uint32_t vkExtensionCount,
    const char* const* vkExtensionNames)
{
    VkInstanceCreateInfo vkCreateInfo{};
    const VkApplicationInfo vkAppInfo = this->game->getVkInfo();

    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &vkAppInfo;

    vkCreateInfo.enabledExtensionCount = vkExtensionCount;
    vkCreateInfo.ppEnabledExtensionNames = vkExtensionNames;

    if (DEBUG && VK_ENABLE_VALIDATION_LAYERS)
    {
        if (!supportsValidationLayers())
        {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }

        vkCreateInfo.enabledLayerCount = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
        vkCreateInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
    } else
    {
        vkCreateInfo.enabledLayerCount = 0;
    }

    VkResult instCreateResult = vkCreateInstance(&vkCreateInfo, nullptr, &this->vkInst);
    if (instCreateResult != VK_SUCCESS) instCreateResult = tryMacFix(vkCreateInfo, &this->vkInst);

    return instCreateResult;
}

VkResult Engine::Renderer::createDevice(VkSurfaceKHR surface)
{
    if (this->vkInst == VK_NULL_HANDLE)
    {
        return VK_ERROR_UNKNOWN;
    }

    uint32_t nDevices;
    vkEnumeratePhysicalDevices(this->vkInst, &nDevices, nullptr);

    std::vector<VkPhysicalDevice> devices(nDevices);
    vkEnumeratePhysicalDevices(this->vkInst, &nDevices, devices.data());

    std::multimap<uint32_t, VkPhysicalDevice> candidates;

    for (const auto& dev : devices)
    {
        candidates.insert(std::make_pair(deviceScore(dev, surface), dev));
    }

    auto physDevice = candidates.rbegin();

    if (physDevice->first == 0)
    {
        vkDestroyInstance(this->vkInst, nullptr);
        this->vkInst = VK_NULL_HANDLE;

        return VK_ERROR_DEVICE_LOST;
    }

    auto familyIndices = vkQueueIndices(physDevice->second, surface);
    std::vector<VkDeviceQueueCreateInfo> qCreateInfos;
    float qPriority = 1.0f;

    for (const auto& index : familyIndices.set())
    {
        VkDeviceQueueCreateInfo qCreateInfo{};
        qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qCreateInfo.pQueuePriorities = &qPriority;
        qCreateInfo.queueFamilyIndex = index;
        qCreateInfo.queueCount = 1;

        qCreateInfos.push_back(qCreateInfo);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = qCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(qCreateInfos.size());
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(VK_DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = VK_DEVICE_EXTENSIONS.data();
    createInfo.enabledLayerCount = 0; // device-level layers are deprecated

    VkResult devCreateResult = vkCreateDevice(physDevice->second, &createInfo, nullptr, &this->vkDev);

    if (devCreateResult != VK_SUCCESS)
    {
        vkDestroyInstance(this->vkInst, nullptr);
        this->vkInst = VK_NULL_HANDLE;

        return devCreateResult;
    }

    vkGetDeviceQueue(this->vkDev, familyIndices.graphicsFamily.value(), 0, &this->vkDevQueue);
    vkGetDeviceQueue(this->vkDev, familyIndices.presentFamily.value(), 0, &this->vkPresentQueue);

    this->vkPhysDev = physDevice->second;
    this->vkSurface = surface;

    return VK_SUCCESS;
}

VkResult Engine::Renderer::createSwapchain(const uint32_t width, const uint32_t height)
{
    if (this->vkInst == VK_NULL_HANDLE || this->vkDev == VK_NULL_HANDLE)
    {
        return VK_ERROR_UNKNOWN;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->vkSurface;

    VkQueueIndices queueIndices = vkQueueIndices(this->vkPhysDev, this->vkSurface);
    uint32_t queueIndexArr[] = {queueIndices.graphicsFamily.value(), queueIndices.presentFamily.value()};

    if (queueIndices.graphicsFamily != queueIndices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndexArr;
    } else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;

    createInfo.imageExtent = {width, height};

    VkSwapChainDetails swapChainDetails = vkQuerySwapchain(this->vkPhysDev, this->vkSurface);
    vkSetupSwapchainCreateInfo(this, createInfo, swapChainDetails);

    // TODO: resizing;
    // "This is a complex topic that we'll learn more about in a future chapter.
    // For now we'll assume that we'll only ever create one swap chain."
    //      - vulkan-tutorial.com
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    this->vkFormat = createInfo.imageFormat;
    this->vkExtent = createInfo.imageExtent;

    VkResult result = vkCreateSwapchainKHR(this->vkDev, &createInfo, nullptr, &this->vkSwapchain);

    if (result == VK_SUCCESS)
    {
        vkGetSwapchainImages(this->vkDev, this->vkSwapchain, &this->vkImages);
        return this->createImageViews();
    }

    return result;
}

VkResult Engine::Renderer::createImageViews()
{
    // I'm kinda lost atp, but trust the process
    // will move into 3D later

    this->vkImageViews.resize(this->vkImages.size());
    uint32_t i = 0;

    for (const auto& image : this->vkImages)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = this->vkFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;

        VkResult result = vkCreateImageView(this->vkDev, &createInfo, nullptr, &this->vkImageViews[i]);

        if (result != VK_SUCCESS) return result;
        i++;
    }

    return VK_SUCCESS;
}

void Engine::Renderer::destroySwapchain()
{
    if (this->vkSwapchain != VK_NULL_HANDLE)
    {
        for (const auto& imageView : this->vkImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(this->vkDev, imageView, nullptr);
            }
        }

        vkDestroySwapchainKHR(this->vkDev, this->vkSwapchain, nullptr);
        this->vkSwapchain = VK_NULL_HANDLE;
    }
}

Engine::Renderer::~Renderer()
{
    this->destroySwapchain();

    if (this->vkDev != VK_NULL_HANDLE)
    {
        vkDestroyDevice(this->vkDev, nullptr);

        this->vkPresentQueue = VK_NULL_HANDLE;
        this->vkDevQueue = VK_NULL_HANDLE;

        this->vkPhysDev = VK_NULL_HANDLE;
        this->vkDev = VK_NULL_HANDLE;
    }

    if (this->vkSurface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(this->vkInst, this->vkSurface, nullptr);
        this->vkSurface = VK_NULL_HANDLE;
    }

    if (this->vkInst != VK_NULL_HANDLE)
    {
        vkDestroyInstance(this->vkInst, nullptr);
        this->vkInst = VK_NULL_HANDLE;
    }
}

#endif