#include <SDL.h>
#include <SDL_vulkan.h>

#include <cstdio>
#include <array>
#include <vector>
#include <chrono>
#include <thread>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

class Engine{
public:
    inline static const std::string AppName    = "01_InitInstance";
    inline static const std::string EngineName = "Vulkan.hpp";

    inline static const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool enableValidationLayers{true};

    SDL_Window *_window;
    vk::Instance _instance;
    vk::Device _device;
    vk::SurfaceKHR _surface;

    Engine(bool validationLayers=true) : enableValidationLayers(validationLayers){
        initSDL();
        initVulkan();
    }

    ~Engine(){
        _device.destroy();
        _instance.destroy();
        SDL_DestroyWindow(_window);
    }

private:

    bool checkValidationLayerSupport() {
        auto[result, availableLayers] = vk::enumerateInstanceLayerProperties();

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
        printf("yo\n");
        SDL_Vulkan_GetInstanceExtensions(_window, &pCount, extensions.data());

        for(const char *extension : extensions){
            printf("Extension: %s\n", extension);
        }
        printf("yo\n");

        return extensions;

    }

    void initSDL(){
        int err;
        err = SDL_Init(SDL_INIT_VIDEO);
        assert(err >= 0);
        err = SDL_Vulkan_LoadLibrary(nullptr);
        assert(err >= 0);

        SDL_Window *_window = SDL_CreateWindow(
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

    void initVulkan(){
        if(enableValidationLayers && !checkValidationLayerSupport()){
            printf("Validation not suported\n");
            abort();
        }

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
        auto resultValue = vk::createInstance( instanceCreateInfo );
        vk::resultCheck(resultValue.result, "Failed to create Vulkan Instance");
        _instance = resultValue.value;

        VkSurfaceKHR srfc;
        SDL_Vulkan_CreateSurface(_window, _instance, &srfc);
        _surface = vk::SurfaceKHR(srfc);

        // vk::DebugUtilsMessengerCreateInfoEXT debugMessInfo;

        //auto [dresult, debugUtilsMessenger] = instance.createDebugUtilsMessengerEXT( debugMessInfo );
        //vk::resultCheck(dresult, "Failed to create debugUtilsMessenger");  

        // enumerate the physicalDevices
        auto [presult, physicalDevices] = _instance.enumeratePhysicalDevices();
        vk::resultCheck(presult, "asd");

        vk::PhysicalDevice physicalDevice = physicalDevices.front();

        // get device with graphics
        auto queueFamilyPrVec = physicalDevice.getQueueFamilyProperties();
        auto propertyIterator = std::find_if( 
            queueFamilyPrVec.begin(),
            queueFamilyPrVec.end(),
            []( vk::QueueFamilyProperties const & qfp ) { 
                return qfp.queueFlags & vk::QueueFlagBits::eGraphics; 
            } 
        );

        size_t graphicsQueueFamilyIndex = std::distance( queueFamilyPrVec.begin(), propertyIterator );
        assert( graphicsQueueFamilyIndex < queueFamilyPrVec.size() );

        // Create device
        float                     queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo( vk::DeviceQueueCreateFlags(), static_cast<uint32_t>( graphicsQueueFamilyIndex ), 1, &queuePriority );
        auto devResultValue = physicalDevice.createDevice( vk::DeviceCreateInfo( vk::DeviceCreateFlags(), deviceQueueCreateInfo ) );
        vk::resultCheck(devResultValue.result, "asd2");
        _device = devResultValue.value;

    }

};


int main(int argc, char* argv[]){
    
    Engine engine;
    printf("finish\n");

    return 0;
}
