#include <SDL.h>
#include <SDL_vulkan.h>

#include <cstdio>
#include <array>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <thread>
#include <optional>

#include <vulkan/vulkan.hpp>

class Engine{
public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> computeFamily;
        std::optional<uint32_t> transferFamily;
    };

    inline static const std::string AppName    = "01_InitInstance";
    inline static const std::string EngineName = "Vulkan.hpp";

    inline static const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool enableValidationLayers{true};

    SDL_Window *_window;
    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice; 
    vk::Device _device;
    vk::SurfaceKHR _surface;
    vk::Queue _graphicsQueue;

    Engine(bool validationLayers=true) : enableValidationLayers(validationLayers){
        initSDL();
        initVulkan();
    }

    ~Engine(){
        _device.destroy();
        _instance.destroySurfaceKHR(_surface);
        _instance.destroy();
        SDL_DestroyWindow(_window);
    }

private:

    bool checkValidationLayerSupport() {
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

    std::vector<const char*> getRequiredExtensions(){
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

    void initSDL(){
        int err;
        err = SDL_Init(SDL_INIT_VIDEO);
        assert(err >= 0);
        err = SDL_Vulkan_LoadLibrary(nullptr);
        assert(err >= 0);

        _window = SDL_CreateWindow(
            "Vulkan Engine",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
        );
        assert(_window!=nullptr);
        printf("init sdl\n");
    }

    std::optional<vk::PhysicalDevice> getSuitablePhysicalDevice(){
        // enumerate the physicalDevices
        auto physicalDevices = _instance.enumeratePhysicalDevices();

        std::optional<vk::PhysicalDevice> descDev = std::nullopt, intDev = std::nullopt;

        for(vk::PhysicalDevice phDev : physicalDevices){
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


    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
        QueueFamilyIndices indices;
        // Logic to find queue family indices to populate struct with
        std::vector<vk::QueueFamilyProperties> qfps = device.getQueueFamilyProperties();
        int i = 0;

        for(const auto& queueFamily : qfps){
            if(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                indices.graphicsFamily = std::optional(i);
        }
        return indices;
    }

    void initVulkan(){
        if(enableValidationLayers && !checkValidationLayerSupport())
            throw std::runtime_error("Validation not suported");

        std::vector<const char*> extensions = getRequiredExtensions();        

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

        QueueFamilyIndices inds = findQueueFamilies(_physicalDevice);
        if(!inds.graphicsFamily.has_value())
            throw std::runtime_error("found no graphics queue");

        // Create device
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
            vk::DeviceQueueCreateFlags(), 
            inds.graphicsFamily.value(), 
            1, 
            &queuePriority 
        );

        vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(
            vk::DeviceCreateFlags(), 
            deviceQueueCreateInfo
        );

        _device = _physicalDevice.createDevice(deviceCreateInfo);

        _graphicsQueue = _device.getQueue(inds.graphicsFamily.value(),0);

    }

};


int main(int argc, char* argv[]){
    
    Engine engine;
    printf("finish\n");

    return 0;
}
