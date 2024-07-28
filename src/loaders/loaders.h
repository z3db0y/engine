#pragma once
#include <vector>

#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace Loaders
{
    bool readfile(std::vector<char>* buffer, const char* filename);

#ifdef HAS_VULKAN

    VkResult loadShaderModule(VkDevice device, const char* filename, VkShaderModule* pShader);

#endif

}
