# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("vulkan_test")

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")


#add_library(sdl2 INTERFACE)
#set(sdl2_DIR ${CMAKE_SOURCE_DIR}/SDL2-2.26.5)
#
#target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include)
#target_link_libraries(sdl2 INTERFACE ${sdl2_DIR}/lib/x64/SDL2.lib ${sdl2_DIR}/lib/x64/SDL2main.lib)
add_subdirectory(third_party)

add_subdirectory(src)

