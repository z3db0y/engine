#pragma once
#include "engine.h"

class Engine::Game
{
    const char* name;
    const int* version;

public:
    Game(const char* name, const int version[3]);

    [[nodiscard]] const char* getName() const;
    [[nodiscard]] const int* getVersion() const;

#ifdef HAS_VULKAN
    [[nodiscard]] VkApplicationInfo getVkInfo() const;
#endif
};
