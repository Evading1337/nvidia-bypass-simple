cmake_minimum_required(VERSION 3.16)
project(NVIDIA_Display_Container C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NOLOGO")

add_library(hook_dll SHARED
    hook_dll.cpp
    minhook/src/buffer.c
    minhook/src/hook.c
    minhook/src/trampoline.c
    minhook/src/hde/hde64.c
)
target_include_directories(hook_dll PRIVATE
    minhook/include
    minhook/src
    minhook/src/hde
    ${CMAKE_SOURCE_DIR}
)

add_executable(nvservice WIN32 nvservice.cpp)
target_include_directories(nvservice PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(nvservice PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)
