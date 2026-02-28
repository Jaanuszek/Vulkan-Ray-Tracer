#include "pch.h"
#include "GUI.hpp"

namespace VRTR
{
    GUI::GUI(RendererContext& ctx)
        : ctx(ctx)
    {
        VmaAllocationCreateInfo allocInfo{
            .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vertexBuffer = std::make_unique<Buffer>(
            ctx.logicalDevice, ctx.vmaAllocator, 1,
            vk::BufferUsageFlagBits::eVertexBuffer,
            allocInfo
        );

        indexBuffer = std::make_unique<Buffer>(
            ctx.logicalDevice, ctx.vmaAllocator, 1,
            vk::BufferUsageFlagBits::eIndexBuffer,
            allocInfo
        );

        renderingInfo.colorAttachmentCount = 1;
        std::array<vk::Format, 1> formats = {colorFormat};
        renderingInfo.pColorAttachmentFormats = formats.data();
    }

    GUI::~GUI()
    {
        if(ctx.logicalDevice != nullptr)
        {
            ctx.logicalDevice.waitIdle();
        }
    }

    void GUI::init(float width, float height)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        // Ewentualnie dodac obsluge Dockowania, w takim przypadku trzeba przelaczyc imgui na branch docking

        io.DisplaySize = ImVec2(width, height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

        vulkanStyle = ImGui::GetStyle();
        vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

        setStyle(0);
    }

    void GUI::initResources()
    {
        ImGuiIO &io = ImGui::GetIO();
        unsigned char *fontData;
        int texWidth, texHeight;
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

        // 1 pixel = 4 bytes (RGBA)
        // ImGui zamienia czcionke na teksture, która jest zoptymalizowana pod GPU
        vk::DeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

        vk::Extent3D fontExtent{
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight),
            1
        };
        fontImage = std::make_unique<Image>(ctx, vk::Format::eR8G8B8A8Unorm);
        fontImage->createImage(fontExtent, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
        fontImage->createImageMemory(vk::MemoryPropertyFlagBits::eDeviceLocal);
        fontImage->createImageView(vk::ImageAspectFlagBits::eColor);

        // Staging buffer
        vk::BufferCreateInfo stagingBufferCI{
            .size = uploadSize,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive
        };

        VmaAllocationCreateInfo stagingBufferAllocInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(ctx.vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&stagingBufferCI), &stagingBufferAllocInfo, &stagingBuffer, &alloc, &allocInfo);

        memcpy(allocInfo.pMappedData, fontData, static_cast<size_t>(uploadSize));

        fontImage->transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        fontImage->copyImageFromStagingToGPU(stagingBuffer);
        fontImage->transitionImageLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        vmaFreeMemory(ctx.vmaAllocator, alloc);
    }

    void GUI::setStyle(uint32_t index)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        switch (index)
        {
        case 0:
            style = vulkanStyle;
            break;
        case 1:
            ImGui::StyleColorsDark();
            break;
        case 2:
            ImGui::StyleColorsClassic();
            break;
        case 3:
            ImGui::StyleColorsLight();
            break;
        default:
            style = vulkanStyle;
            break;
        }
    }

}