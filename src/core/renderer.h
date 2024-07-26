#pragma once
#include "engine.h"

#ifdef HAS_VULKAN

class Engine::Renderer
{
    Game* game;

    VkPhysicalDevice vkPhysDev{};
    VkDevice vkDev{};
    VkQueue vkDevQueue{};
    VkQueue vkPresentQueue{};

    VkFormat vkFormat{};
    VkExtent2D vkExtent{};

    VkSwapchainKHR vkSwapchain{};
    VkSurfaceKHR vkSurface{};

    VkViewport vkViewport{};
    VkRect2D vkScissor{};

    VkRenderPass vkRenderPass{};
    VkPipelineLayout vkLayout{};
    VkPipeline vkPipeline{};

    std::vector<VkImage> vkImages;
    std::vector<VkImageView> vkImageViews;

    VkResult createImageViews();
    VkResult createRenderPass();
public:
    VkInstance vkInst{};
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    explicit Renderer(Game* game);
    VkResult createInstance(uint32_t vkExtensionCount, const char* const* vkExtensionNames);
    VkResult createDevice(VkSurfaceKHR surface);

    VkResult createSwapchain(uint32_t width, uint32_t height);
    void cleanupSwapchain();

    VkResult createRenderPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaders);

    ~Renderer();
};

#endif