#include "loaders.h"
#include <fstream>

bool Loaders::readfile(std::vector<char>* buffer, const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    const size_t size = file.tellg();
    buffer->resize(size);

    file.seekg(0);
    file.read(buffer->data(), static_cast<std::streamsize>(size));

    file.close();
    return true;
}
