#include "pch.h"
#include "GUI.hpp"
#include "CommandBufferManager.hpp"

namespace VRTR
{
    GUI::GUI(RendererContext& ctx, SceneSettings &sceneSettings)
        : ctx(ctx), sceneSettings(sceneSettings)
    {}

    GUI::~GUI()
    {
        VRTR_DEBUG("Destroying GUI");
        if (ctx.logicalDevice != nullptr)
        {
            ctx.logicalDevice.waitIdle();
        }
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
    }

    void GUI::init(GLFWwindow* window, float width, float height)
    {
        this->window = window;
        mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

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
        setupStyle();

        ImGui_ImplGlfw_InitForVulkan(window, false);
    }

    void GUI::initResources(vk::raii::CommandPool& commandPool, vk::Format swapchainFormat, uint32_t imageCount)
    {
        this->swapchainFormat = static_cast<VkFormat>(swapchainFormat);
        this->imageCount = imageCount;

        std::array<vk::DescriptorPoolSize, 1> poolSizes = {
            vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1024}
        };
        vk::DescriptorPoolCreateInfo poolCI{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1024,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };
        descriptorPool = ctx.logicalDevice.createDescriptorPool(poolCI);

        pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingInfo.colorAttachmentCount = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &this->swapchainFormat;

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = VK_API_VERSION_1_4;
        initInfo.Instance = *ctx.instance;
        initInfo.PhysicalDevice = *ctx.gpu;
        initInfo.Device = *ctx.logicalDevice;
        initInfo.QueueFamily = static_cast<uint32_t>(ctx.graphics_queue_index);
        initInfo.Queue = *ctx.queue;
        initInfo.DescriptorPool = *descriptorPool;
        initInfo.MinImageCount = std::max(2u, imageCount);
        initInfo.ImageCount = imageCount;
        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;

        ImGui_ImplVulkan_Init(&initInfo);

        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = imageCount
        };
        guiCommandBuffers = ctx.logicalDevice.allocateCommandBuffers(allocInfo);

        // vk::raii::CommandBuffer uploadCmd{nullptr};
        // COMMANDS::beginSingleTimeCommands(uploadCmd, ctx.logicalDevice, commandPool);
        // ImGui_ImplVulkan_CreateFontsTexture(*uploadCmd);
        // COMMANDS::endSingleTimeCommands(uploadCmd, ctx.logicalDevice, commandPool, ctx.queue);
        // ImGui_ImplVulkan_DestroyFontUploadObjects();
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

    void GUI::setupStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.ScaleAllSizes(mainScale);
        style.FontScaleDpi = mainScale;
    }

    bool GUI::newFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show the demo window
        ImGui::ShowDemoWindow();

        // Tutaj trzeba dodac wlasne GUI np:
        ImGui::Begin("Another Window", nullptr);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me")) {
            ;
        }
        if(ImGui::SliderFloat("Rotation Angle", &sceneSettings.transformations.rotationAngle, 0.0f, 360.0f))
        {
            updated = true;
        }
        if(ImGui::SliderFloat3("Light Position", &sceneSettings.ubo.light_pos.x, -10.0f, 10.0f))
        {
            updated = true;
        }
        if(ImGui::Button("Enable CUDA"))
        {
            sceneSettings.ubo.enableCUDA = !sceneSettings.ubo.enableCUDA;
            updated = true;
        }
        if(ImGui::Button("Swap to shadows"))
        {
            sceneSettings.ubo.shadowMode = !sceneSettings.ubo.shadowMode;
            updated = true;
        }
        if(ImGui::Button("Debug Patches"))
        {
            sceneSettings.ubo.debugPatches = !sceneSettings.ubo.debugPatches;
            updated = true;
        }
        ImGui::Text("FPS %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Render();

        ImDrawData *drawData = ImGui::GetDrawData();
        return drawData && drawData->CmdListsCount > 0;
    }

    vk::CommandBuffer GUI::buildDrawCommandBuffer(uint32_t imageIndex, vk::Image swapchainImage, vk::ImageView swapchainImageView, vk::Extent2D extent)
    {
        ImDrawData *drawData = ImGui::GetDrawData();
        if (!drawData || drawData->CmdListsCount == 0)
        {
            return VK_NULL_HANDLE;
        }
        if(guiCommandBuffers.empty())
        {
            return VK_NULL_HANDLE;
        }
        auto& commandBuffer = guiCommandBuffers.at(imageIndex);
        commandBuffer.reset();
        commandBuffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        CommandBufferManager::transition_image_layout(
            commandBuffer,
            swapchainImage,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );

        vk::RenderingAttachmentInfo colorAttachment{
            .imageView = swapchainImageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore
        };

        vk::RenderingInfo renderingInfo{
            .renderArea = vk::Rect2D{
                .offset = vk::Offset2D{0,0},
                .extent = extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        };
        commandBuffer.beginRendering(renderingInfo);
        ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffer);
        commandBuffer.endRendering();

        CommandBufferManager::transition_image_layout(
            commandBuffer,
            swapchainImage,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        );

        commandBuffer.end();
        return commandBuffer;
    }
}