#include "vkVLayers.h"

#include <cstring>
#include <vector>

#include "engine.h"
#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>

bool supportsValidationLayers()
{
    uint32_t nLayers;
    vkEnumerateInstanceLayerProperties(&nLayers, nullptr);

    std::vector<VkLayerProperties> layers(nLayers);
    vkEnumerateInstanceLayerProperties(&nLayers, layers.data());

    for (const auto& required : Engine::VK_VALIDATION_LAYERS)
    {
        bool found = false;

        for (const auto& layer : layers)
        {
            found = strcmp(layer.layerName, required);
            if (found) break;
        }

        if (!found) return false;
    }

    return true;
}

#endif
