#include "loaders.h"
#ifdef HAS_VULKAN

VkResult Loaders::loadShaderModule(VkDevice device, const char* filename, VkShaderModule* pShader)
{
    std::vector<char> buffer;

    if (!readfile(&buffer, filename))
    {
        return VK_ERROR_UNKNOWN;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<uint32_t*>(buffer.data());

    return vkCreateShaderModule(device, &createInfo, nullptr, pShader);
}

#endif