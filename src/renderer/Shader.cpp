#include "pch.h"
#include "Shader.hpp"

namespace VRTR
{
    std::vector<char> Shader::readFile(const std::string& filename)
    {
        VRTR_DEBUG("Reading shader file: {}", filename);
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if(!file.is_open())
        {
            VRTR_ERROR("Failed to open file: {}", filename);
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::vector<char> buffer(file.tellg());

        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close(); // it's not needed, but it's good practice to close the file

        return buffer;
    }

    [[nodiscard]] vk::raii::ShaderModule Shader::createShaderModule(vk::raii::Device& device,
                                                                    const std::vector<char>& code) const
    {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };

        return vk::raii::ShaderModule{device, createInfo};
    }
}