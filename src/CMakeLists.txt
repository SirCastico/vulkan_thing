# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

find_package(Vulkan REQUIRED)

add_executable(test main.cpp)

# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(test PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(test PRIVATE glm::glm SDL2::SDL2 Vulkan::Vulkan VulkanMemoryAllocator )
