#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "SDL_events.h"
#include "SDL_keycode.h"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <array>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <optional>
#include <glm/common.hpp>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> presentFamily;

    bool isGraphicsAndPresentEqual(){
        return graphicsFamily.has_value() && presentFamily.has_value() ? 
            graphicsFamily.value() == presentFamily.value() : false;
    }
};

class Renderer{
public:

    inline static const std::string AppName    = "01_InitInstance";
    inline static const std::string EngineName = "Vulkan.hpp";

    inline static const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    inline static const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool enableValidationLayers{true};

    SDL_Window *_window;
    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice; 
    vk::Device _device;
    vk::SurfaceKHR _surface;
    vk::Queue _graphicsQueue;

    vk::SwapchainKHR _swapchain;
    vk::Format _swapchainFormat;
    std::vector<vk::Image> _swapchainImages;
    std::vector<vk::ImageView> _swapchainImageViews;

    vk::Extent2D _window_size{800,600}, _swapchainExtent;
    
    vk::CommandPool _commandPool;
    vk::CommandBuffer _commandBuffer;

    vk::RenderPass _renderPass;
    std::vector<vk::Framebuffer> _frameBuffers;

    vk::Fence _renderFence;
    vk::Semaphore _presentSemaphore;
    vk::Semaphore _renderSemaphore;
    
    QueueFamilyIndices _queueIndices;

    uint64_t _frameNumber{0};

    Renderer(bool validationLayers=true);

    ~Renderer();

    void draw();

private:

    bool checkValidationLayerSupport();

    std::vector<const char*> getSDLRequiredExtensions();

    void initSDL();

    void initVulkan();

    void initSyncStructures();

    void initFrameBuffers();

    void initDefaultRenderPass();

    void initSwapchain();

    void initCommands();

    bool checkDeviceExtensions(vk::PhysicalDevice ph);

    std::optional<vk::PhysicalDevice> getSuitablePhysicalDevice();


    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

};