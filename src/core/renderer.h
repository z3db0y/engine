#pragma once
#include "engine.h"

#ifdef HAS_VULKAN

class Engine::Renderer
{
    Game* game;

    VkPhysicalDevice vkPhysDev = VK_NULL_HANDLE;

    VkQueue vkGraphicsQueue = VK_NULL_HANDLE;
    VkQueue vkPresentQueue = VK_NULL_HANDLE;

    VkFormat vkFormat{};
    VkExtent2D vkExtent{};

    VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

    VkViewport vkViewport{};
    VkRect2D vkScissor{};

    VkRenderPass vkRenderPass = VK_NULL_HANDLE;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    VkPipeline vkPipeline = VK_NULL_HANDLE;

    VkCommandPool vkCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer vkCmdBuffer = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    std::vector<VkImage> vkImages;
    std::vector<VkImageView> vkImageViews;
    std::vector<VkFramebuffer> vkFramebuffers;

    VkResult createImageViews();
    VkResult createFramebuffers();
    VkResult createSyncObjects();
    VkResult createRenderPass();

    VkResult recordCommandBuffer(uint32_t imageIndex);
public:
    VkInstance vkInst = VK_NULL_HANDLE;
    VkDevice vkDev = VK_NULL_HANDLE;

    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    explicit Renderer(Game* game);
    VkResult createInstance(uint32_t vkExtensionCount, const char* const* vkExtensionNames);
    VkResult createDevice(VkSurfaceKHR surface);

    VkResult createSwapchain(uint32_t width, uint32_t height);
    void cleanupSwapchain();

    VkResult createRenderPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaders);
    VkResult render();

    ~Renderer();
};

#endif