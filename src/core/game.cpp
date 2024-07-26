#include "game.h"

Engine::Game::Game(const char* name, const int version[3])
{
    this->name = name;
    this->version = version;
}

const char* Engine::Game::getName() const
{
    return this->name;
}

const int* Engine::Game::getVersion() const
{
    return this->version;
}

#ifdef HAS_VULKAN

VkApplicationInfo Engine::Game::getVkInfo() const
{
    auto info = VkApplicationInfo{};

    info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    info.pApplicationName = this->name;
    info.applicationVersion = VK_MAKE_VERSION(
        this->version[0],
        this->version[1],
        this->version[2]
    );

    info.pEngineName = NAME;
    info.engineVersion = VK_MAKE_VERSION(
        VERSION[0],
        VERSION[1],
        VERSION[2]
    );

    info.apiVersion = VK_API_VERSION_1_0;
    return info;
}


#endif
