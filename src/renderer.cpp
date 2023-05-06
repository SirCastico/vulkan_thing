#include "renderer.hpp"
#include <vulkan/vulkan.hpp>

Renderer::Renderer(bool validationLayers) : enableValidationLayers(validationLayers){
    initSDL();
    initVulkan();
}

Renderer::~Renderer(){
    vk::resultCheck(
        _device.waitForFences(_renderFence, true, 1000000000),
        "wait for fence"
    );
    _device.destroyFence(_renderFence);
    _device.destroySemaphore(_renderSemaphore);
    _device.destroySemaphore(_presentSemaphore);
    for(int i=0;i<_frameBuffers.size();++i){
        _device.destroyFramebuffer(_frameBuffers[i]);
    }
    _device.destroyRenderPass(_renderPass);
    _device.destroyCommandPool(_commandPool);
    for(auto im : _swapchainImageViews)
        _device.destroyImageView(im);
    _device.destroySwapchainKHR(_swapchain);
    _device.destroy();
    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
    SDL_DestroyWindow(_window);
}

void Renderer::draw(){
    printf("frame:%ld\n",_frameNumber);
    // Timeout 1 second
    vk::resultCheck(
        _device.waitForFences(_renderFence, true, 1000000000),
        "wait for fence"
    );
    _device.resetFences(_renderFence);

    auto [swapImageResult, swapImageInd] = _device.acquireNextImageKHR(_swapchain,1000000000,_presentSemaphore,{});
    vk::resultCheck(swapImageResult,"swapchain image");
    _commandBuffer.reset();

    // Prepare cmd
    vk::CommandBufferBeginInfo cmdBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    _commandBuffer.begin(cmdBeginInfo);

    vk::ClearValue clVal{
        {
            0.0f,
            0.0f,
            std::abs(std::sin(_frameNumber/120.f)),
            0.0f
        }
    };
    vk::RenderPassBeginInfo rpInfo{};
    rpInfo.renderPass = _renderPass;
    rpInfo.renderArea = vk::Rect2D{
        {0,0},
        _window_size
    };
    rpInfo.framebuffer = _frameBuffers[swapImageInd];
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clVal;

    _commandBuffer.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);
    _commandBuffer.endRenderPass();
    _commandBuffer.end();

    // submit cmd
    vk::SubmitInfo submit{};
    vk::PipelineStageFlags waitStage{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit.pWaitDstStageMask = &waitStage; //??

    submit.setWaitSemaphores(_presentSemaphore);
    submit.setSignalSemaphores(_renderSemaphore);
    submit.setCommandBuffers(_commandBuffer);

    _graphicsQueue.submit(submit,_renderFence);

    // present render
    vk::PresentInfoKHR presentInfo{};
    presentInfo.setSwapchains(_swapchain);
    presentInfo.setWaitSemaphores(_renderSemaphore);
    presentInfo.setImageIndices(swapImageInd);
    vk::resultCheck(_graphicsQueue.presentKHR(presentInfo),"failed present");
    ++_frameNumber;
}


bool Renderer::checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for(const char *layerName : validationLayers) {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers){
            if(std::strcmp(layerName, layerProperties.layerName)==0){
                layerFound = true;
                break;
            }
        }
        if(!layerFound)
            return false;
    }

    return true;
}

std::vector<const char*> Renderer::getSDLRequiredExtensions(){
    // Get Extensions
    uint32_t pCount=0;
    SDL_Vulkan_GetInstanceExtensions(_window, &pCount, nullptr);
    std::vector<const char*> extensions(pCount);    
    SDL_Vulkan_GetInstanceExtensions(_window, &pCount, extensions.data());

    for(const char *extension : extensions){
        printf("Extension: %s\n", extension);
    }

    return extensions;

}

void Renderer::initSDL(){
    int err;
    err = SDL_Init(SDL_INIT_VIDEO);
    assert(err >= 0);
    err = SDL_Vulkan_LoadLibrary(nullptr);
    assert(err >= 0);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        _window_size.width,
        _window_size.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );
    assert(_window!=nullptr);
    printf("init sdl\n");
}



void Renderer::initVulkan(){
    if(enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation not suported");

    std::vector<const char*> extensions = getSDLRequiredExtensions();        

    // initialize the vk::ApplicationInfo structure
    vk::ApplicationInfo applicationInfo( AppName.c_str(), 1, EngineName.c_str(), 1, VK_API_VERSION_1_1 );

    // initialize the vk::InstanceCreateInfo
    vk::InstanceCreateInfo instanceCreateInfo = {
        {}, 
        &applicationInfo, 
        {}, 
        extensions
    };
    if(enableValidationLayers)
        instanceCreateInfo.setPEnabledLayerNames(validationLayers);


    // create an Instance
    _instance = vk::createInstance( instanceCreateInfo );

    {
        VkSurfaceKHR surf;
        if(!SDL_Vulkan_CreateSurface(_window, _instance, &surf))
            throw std::runtime_error("Failed to create Surface");
        _surface = surf;
    }

    auto optPhysicalDevice = getSuitablePhysicalDevice();
    if(!optPhysicalDevice.has_value())
        throw std::runtime_error("no suitable device found");

    _physicalDevice = optPhysicalDevice.value();

    _queueIndices = findQueueFamilies(_physicalDevice);

    if(!_queueIndices.graphicsFamily.has_value())
        throw std::runtime_error("found no graphics queue");

    // Create device
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(), 
        _queueIndices.graphicsFamily.value(), 
        1, 
        &queuePriority 
    );

    vk::DeviceCreateInfo deviceCreateInfo(
        vk::DeviceCreateFlags(), 
        deviceQueueCreateInfo,
        {},
        deviceExtensions
    );

    _device = _physicalDevice.createDevice(deviceCreateInfo);

    _graphicsQueue = _device.getQueue(_queueIndices.graphicsFamily.value(),0);

    initSwapchain();

    initCommands();
    
    initDefaultRenderPass();

    initFrameBuffers();

    initSyncStructures();
}

void Renderer::initSyncStructures(){
    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};
    _renderFence = _device.createFence(fenceInfo);

    vk::SemaphoreCreateInfo semInfo{};
    _presentSemaphore = _device.createSemaphore(semInfo);
    _renderSemaphore = _device.createSemaphore(semInfo);
}

void Renderer::initFrameBuffers(){
    //create the framebuffers for the swapchain images. 
    //This will connect the render-pass to the images for rendering
    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = _renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.width = _window_size.width;
    fbInfo.height = _window_size.height;
    fbInfo.layers = 1;
    
    _frameBuffers = std::vector<vk::Framebuffer>(_swapchainImages.size());

    for(int i=0;i<_swapchainImages.size();++i){
        fbInfo.pAttachments = &_swapchainImageViews[i];
        auto fbResult = _device.createFramebuffer(
            &fbInfo, 
            nullptr, 
            &_frameBuffers[i]
        );
        vk::resultCheck(fbResult, "bad frame buffer");
    }

}

void Renderer::initDefaultRenderPass(){
    // the renderpass will use this color attachment.
    vk::AttachmentDescription colorAtt{};
    //the attachment will have the format needed by the swapchain
    colorAtt.format = _swapchainFormat;
    //1 sample, we won't be doing MSAA
    colorAtt.samples = vk::SampleCountFlagBits::e1;
    // we Clear when this attachment is loaded
    colorAtt.loadOp = vk::AttachmentLoadOp::eClear;
    // we keep the attachment stored when the renderpass ends
    colorAtt.storeOp = vk::AttachmentStoreOp::eStore;
    //we don't care about stencil
    colorAtt.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAtt.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    //we don't know or care about the starting layout of the attachment
    colorAtt.initialLayout = vk::ImageLayout::eUndefined;

    //after the renderpass ends, the image has to be on a layout ready for display
    colorAtt.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttRef{};
    //attachment number will index into the pAttachments array in the parent renderpass itself
    colorAttRef.attachment = 0;
    colorAttRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    // 1 subpass
    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttRef;

    vk::RenderPassCreateInfo rPassInfo{};
    rPassInfo.attachmentCount = 1;
    rPassInfo.pAttachments = &colorAtt;
    rPassInfo.subpassCount = 1;
    rPassInfo.pSubpasses = &subpass;
    
    auto result = _device.createRenderPass(&rPassInfo,nullptr,&_renderPass);
    vk::resultCheck(result,"failed to create render pass");

}

void Renderer::initSwapchain(){
    // get the supported VkFormats
    std::vector<vk::SurfaceFormatKHR> formats = _physicalDevice.getSurfaceFormatsKHR( _surface );
    assert( !formats.empty() );
    _swapchainFormat = ( formats[0].format == vk::Format::eUndefined ) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = _physicalDevice.getSurfaceCapabilitiesKHR( _surface );
    if ( surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() ){
        // If the surface size is undefined, the size is set to the size of the images requested.
        _swapchainExtent.width  = glm::clamp(
            _window_size.width,
            surfaceCapabilities.minImageExtent.width, 
            surfaceCapabilities.maxImageExtent.width 
        );
        _swapchainExtent.height = glm::clamp(
            _window_size.height,
            surfaceCapabilities.minImageExtent.height, 
            surfaceCapabilities.maxImageExtent.height 
        );
    }
    else{
        // If the surface size is defined, the swap chain size must match
        _swapchainExtent = surfaceCapabilities.currentExtent;
    }

    vk::SurfaceTransformFlagBitsKHR preTransform = ( surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity )
                                                ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                : surfaceCapabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
    ( surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied )    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
    : ( surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
    : ( surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit )        ? vk::CompositeAlphaFlagBitsKHR::eInherit
                                                                                                        : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        vk::SwapchainCreateFlagsKHR(),
        _surface,
        surfaceCapabilities.minImageCount,
        _swapchainFormat,
        vk::ColorSpaceKHR::eSrgbNonlinear,
        _swapchainExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        {},
        preTransform,
        compositeAlpha,
        vk::PresentModeKHR::eFifo,
        true,
        nullptr
    );
    if (!_queueIndices.isGraphicsAndPresentEqual()){
        // If the graphics and present queues are from different queue families, we either have to explicitly transfer
        // ownership of images between the queues, or we have to create the swapchain with imageSharingMode as
        // VK_SHARING_MODE_CONCURRENT
        uint32_t queueFamilyIndices[2] = {_queueIndices.graphicsFamily.value(), _queueIndices.transferFamily.value()};
        swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }

    _swapchain = _device.createSwapchainKHR( swapChainCreateInfo );

    _swapchainImages = _device.getSwapchainImagesKHR( _swapchain );

    _swapchainImageViews.reserve( _swapchainImages.size() );
    vk::ImageViewCreateInfo imageViewCreateInfo(
        {},
        {},
        vk::ImageViewType::e2D,
        _swapchainFormat, 
        {}, 
        {
            vk::ImageAspectFlagBits::eColor, 
            0, 
            1, 
            0, 
            1 
        } 
    );
    for ( auto image : _swapchainImages){
        imageViewCreateInfo.image = image;
        _swapchainImageViews.push_back( _device.createImageView( imageViewCreateInfo ) );
    }
}

void Renderer::initCommands(){
    vk::CommandPoolCreateInfo cPoolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _queueIndices.graphicsFamily.value()
    };
    _commandPool = _device.createCommandPool(cPoolInfo);

    vk::CommandBufferAllocateInfo cBufAllInfo{
        _commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    };
    _commandBuffer = _device.allocateCommandBuffers(cBufAllInfo)[0];

}

bool Renderer::checkDeviceExtensions(vk::PhysicalDevice ph){
    std::vector<vk::ExtensionProperties> exts = ph.enumerateDeviceExtensionProperties();
    for(auto str : deviceExtensions){
        auto it = std::find_if(exts.begin(), exts.end(), [str](vk::ExtensionProperties e){return std::strcmp(str, e.extensionName)==0;});
        if(it==exts.end())
            return false;
    }
    return true;
}

std::optional<vk::PhysicalDevice> Renderer::getSuitablePhysicalDevice(){
    // enumerate the physicalDevices
    auto physicalDevices = _instance.enumeratePhysicalDevices();

    std::optional<vk::PhysicalDevice> descDev = std::nullopt, intDev = std::nullopt;

    for(vk::PhysicalDevice phDev : physicalDevices){
        if(!checkDeviceExtensions(phDev))
            continue;

        vk::PhysicalDeviceProperties pr = phDev.getProperties();
        vk::PhysicalDeviceFeatures ft = phDev.getFeatures();
        if(pr.deviceType == vk::PhysicalDeviceType::eDiscreteGpu){
            descDev = phDev;
            break;
        }
        else if(pr.deviceType == vk::PhysicalDeviceType::eIntegratedGpu){
            intDev = phDev;
            break;
        }
    }
    if(descDev.has_value())
        return descDev;
    else if(intDev.has_value())
        return intDev;

    return std::nullopt;
} 


QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with
    std::vector<vk::QueueFamilyProperties> qfps = device.getQueueFamilyProperties();
    int i = 0;

    for(const auto& queueFamily : qfps){
        if(indices.computeFamily && indices.graphicsFamily && indices.presentFamily && indices.transferFamily)
            break;
        if(!indices.isGraphicsAndPresentEqual() && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics){
            indices.graphicsFamily = std::optional(i);
            if(device.getSurfaceSupportKHR(i, _surface))
                indices.presentFamily = i;
        }
        if(queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
            indices.computeFamily = std::optional(i);

        if(queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
            indices.transferFamily = std::optional(i);
        
        if(!indices.presentFamily)
            if(device.getSurfaceSupportKHR(i, _surface))
                indices.presentFamily = i;

    }
    return indices;
}