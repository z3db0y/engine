#include "renderer.h"

#ifdef HAS_VULKAN

#include <cstring>
#include <map>
#include <vector>

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
        return devCreateResult;
    }

    vkGetDeviceQueue(this->vkDev, familyIndices.graphicsFamily.value(), 0, &this->vkGraphicsQueue);
    vkGetDeviceQueue(this->vkDev, familyIndices.presentFamily.value(), 0, &this->vkPresentQueue);

    this->vkPhysDev = physDevice->second;
    this->vkSurface = surface;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();

    VkResult cmdPoolResult = vkCreateCommandPool(this->vkDev, &poolInfo, nullptr, &this->vkCmdPool);

    if (cmdPoolResult != VK_SUCCESS)
    {
        return cmdPoolResult;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->vkCmdPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return vkAllocateCommandBuffers(this->vkDev, &allocInfo, &this->vkCmdBuffer);
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

    // to past self ^^: you had no clue what you were
    // talking about, didn't you?

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

VkResult Engine::Renderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->vkFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentRef{};
    attachmentRef.attachment = 0;
    attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    // i'm actually so fucking lost rn
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(this->vkDev, &renderPassInfo, nullptr, &this->vkRenderPass);
}

VkResult Engine::Renderer::createFramebuffers()
{
    this->vkFramebuffers.resize(this->vkImageViews.size());

    uint32_t i = 0;
    for (const auto& imageView : this->vkImageViews)
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = this->vkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imageView;
        framebufferInfo.width = this->vkExtent.width;
        framebufferInfo.height = this->vkExtent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(this->vkDev, &framebufferInfo, nullptr, &this->vkFramebuffers[i]);

        if (result != VK_SUCCESS)
        {
            return result;
        }

        i++;
    }

    return VK_SUCCESS;
}

VkResult Engine::Renderer::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result;

    if (
        (result = vkCreateSemaphore(this->vkDev, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore)) != VK_SUCCESS ||
        (result = vkCreateSemaphore(this->vkDev, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore)) != VK_SUCCESS ||
        (result = vkCreateFence(this->vkDev, &fenceInfo, nullptr, &this->inFlightFence)) != VK_SUCCESS)
    {
        return result;
    }


    return VK_SUCCESS;
}

VkResult Engine::Renderer::createRenderPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaders)
{
    // TODO: revisit & configure

    VkPipelineVertexInputStateCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInfo.vertexBindingDescriptionCount = 0;
    vertexInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    std::vector dynStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynState.pDynamicStates = dynStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pViewports = &this->vkViewport;
    viewportState.viewportCount = 1;
    viewportState.pScissors = &this->vkScissor;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.depthBiasEnable = VK_FALSE;

    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkResult layoutResult = vkCreatePipelineLayout(this->vkDev, &layoutInfo, nullptr, &this->vkLayout);
    VkResult renderPassResult = this->createRenderPass();

    if (layoutResult != VK_SUCCESS)
    {
        return layoutResult;
    }

    if (renderPassResult != VK_SUCCESS)
    {
        return renderPassResult;
    }

    VkResult fbResult = this->createFramebuffers();

    if (fbResult != VK_SUCCESS)
    {
        return fbResult;
    }

    VkResult syncObjsResult = this->createSyncObjects();

    if (syncObjsResult != VK_SUCCESS)
    {
        return syncObjsResult;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipelineInfo.stageCount = static_cast<uint32_t>(shaders.size());
    pipelineInfo.pStages = shaders.data();

    pipelineInfo.pVertexInputState = &vertexInfo;
    pipelineInfo.pInputAssemblyState = &assembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynState;

    return vkCreateGraphicsPipelines(
        this->vkDev,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &this->vkPipeline);
}

VkResult Engine::Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    VkCommandBufferBeginInfo bufBeginInfo{};
    bufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result = vkBeginCommandBuffer(this->vkCmdBuffer, &bufBeginInfo);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    VkRenderPassBeginInfo passBeginInfo{};
    passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passBeginInfo.framebuffer = this->vkFramebuffers[imageIndex];
    passBeginInfo.renderPass = this->vkRenderPass;

    passBeginInfo.renderArea.offset = {0, 0};
    passBeginInfo.renderArea.extent = this->vkExtent;

    constexpr VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    passBeginInfo.clearValueCount = 1;
    passBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(this->vkCmdBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(this->vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkPipeline);

    this->vkViewport.x = 0.0f;
    this->vkViewport.y = 0.0f;

    this->vkViewport.width = static_cast<float>(this->vkExtent.width);
    this->vkViewport.height = static_cast<float>(this->vkExtent.height);

    this->vkViewport.minDepth = 0.0f;
    this->vkViewport.maxDepth = 1.0f;

    vkCmdSetViewport(this->vkCmdBuffer, 0, 1, &this->vkViewport);

    this->vkScissor.offset = {0, 0};
    this->vkScissor.extent = this->vkExtent;

    vkCmdSetScissor(this->vkCmdBuffer, 0, 1, &this->vkScissor);

    // TODO: actually count vertices????????????
    vkCmdDraw(this->vkCmdBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(this->vkCmdBuffer);

    return vkEndCommandBuffer(this->vkCmdBuffer);
}

VkResult Engine::Renderer::render()
{
    vkWaitForFences(this->vkDev, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(this->vkDev, 1, &this->inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(this->vkDev, this->vkSwapchain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(this->vkCmdBuffer, 0);
    this->recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &this->imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->vkCmdBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &this->renderFinishedSemaphore;

    VkResult result = vkQueueSubmit(this->vkGraphicsQueue, 1, &submitInfo, this->inFlightFence);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &this->imageAvailableSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &this->vkSwapchain;
    presentInfo.pImageIndices = &imageIndex;

    return vkQueuePresentKHR(this->vkPresentQueue, &presentInfo);
}


void Engine::Renderer::cleanupSwapchain()
{
    if (this->vkSwapchain != VK_NULL_HANDLE)
    {
        if (this->vkRenderPass != VK_NULL_HANDLE)
        {
            for (const auto& framebuffer : this->vkFramebuffers)
            {
                vkDestroyFramebuffer(this->vkDev, framebuffer, nullptr);
            }

            this->vkFramebuffers.resize(0);
        }

        for (const auto& imageView : this->vkImageViews)
        {
            vkDestroyImageView(this->vkDev, imageView, nullptr);
        }

        vkDestroySwapchainKHR(this->vkDev, this->vkSwapchain, nullptr);
        this->vkSwapchain = VK_NULL_HANDLE;
        this->vkImageViews.resize(0);
    }
}

Engine::Renderer::~Renderer()
{
    this->cleanupSwapchain();

    if (this->inFlightFence != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(this->vkDev, this->imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(this->vkDev, this->renderFinishedSemaphore, nullptr);
        vkDestroyFence(this->vkDev, this->inFlightFence, nullptr);

        this->imageAvailableSemaphore = VK_NULL_HANDLE;
        this->renderFinishedSemaphore = VK_NULL_HANDLE;
        this->inFlightFence = VK_NULL_HANDLE;
    }

    if (this->vkPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(this->vkDev, this->vkPipeline, nullptr);
        this->vkPipeline = VK_NULL_HANDLE;
    }

    if (this->vkLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(this->vkDev, this->vkLayout, nullptr);
        this->vkLayout = VK_NULL_HANDLE;
    }

    if (this->vkRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(this->vkDev, this->vkRenderPass, nullptr);
        this->vkRenderPass = VK_NULL_HANDLE;
    }

    if (this->vkDev != VK_NULL_HANDLE)
    {
        vkDestroyDevice(this->vkDev, nullptr);

        this->vkPresentQueue = VK_NULL_HANDLE;
        this->vkGraphicsQueue = VK_NULL_HANDLE;

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