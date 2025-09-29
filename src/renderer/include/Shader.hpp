#pragma once

#include "Logger.hpp"

namespace VRTR
{
    class Shader
    {
        public:
            Shader() = default;
            ~Shader() = default;

            std::vector<char> readFile(const std::string& filename) const;

            [[nodiscard]]  vk::raii::ShaderModule createShaderModule(vk::raii::Device& device,
                                                                    const std::vector<char>& code);

            vk::PipelineShaderStageCreateInfo createShaderStageInfo(vk::raii::Device& device,
                                                                    const std::string& filename,
                                                                    vk::ShaderStageFlagBits stage);
        private:
            std::vector<vk::raii::ShaderModule> shaderModules;
    };
}