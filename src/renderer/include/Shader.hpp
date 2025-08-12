#pragma once

#include "Logger.hpp"

namespace VRTR
{
    class Shader
    {
        public:
            Shader() = default;
            ~Shader() = default;

            std::vector<char> readFile(const std::string& filename);

            [[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device,
                                                                    const std::vector<char>& code) const;
        private:
    };
}