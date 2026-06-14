# CMakeLists.txt for EUI-NEO Android native library
cmake_minimum_required(VERSION 3.22.1)
project(eui-neo-native CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Paths
set(EUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SHIM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp)

# Include directories (shims first to override original GLFW/glad)
include_directories(
    ${SHIM_ROOT}/glfw_shim
    ${SHIM_ROOT}/glad_gles
    ${EUI_ROOT}
    ${EUI_ROOT}/include
    ${EUI_ROOT}/3rd
)

# Source files
set(SHIM_SOURCES
    ${SHIM_ROOT}/glfw_shim/glfw_shim.cpp
)

set(ANDROID_SOURCES
    ${SHIM_ROOT}/android_main.cpp
    ${SHIM_ROOT}/android_app.cpp
)

set(EUI_CORE_SOURCES
    ${EUI_ROOT}/core/platform/async.cpp
    ${EUI_ROOT}/core/platform/ime_bridge.c
    ${EUI_ROOT}/core/platform/network.cpp
    ${EUI_ROOT}/core/platform/native_bridge.c
    ${EUI_ROOT}/core/platform/performance_stats.cpp
    ${EUI_ROOT}/core/platform/platform.cpp
    ${EUI_ROOT}/core/platform/tray_bridge.c
    ${EUI_ROOT}/core/render/image.cpp
    ${EUI_ROOT}/core/render/image_source.cpp
    ${EUI_ROOT}/core/render/primitive.cpp
    ${EUI_ROOT}/core/render/render_backend.cpp
    ${EUI_ROOT}/core/render/stb_image_impl.cpp
    ${EUI_ROOT}/core/render/text.cpp
    ${EUI_ROOT}/core/render/opengl/opengl_backend.cpp
    ${EUI_ROOT}/core/render/opengl/opengl_image.cpp
    ${EUI_ROOT}/core/render/opengl/opengl_primitives.cpp
    ${EUI_ROOT}/core/render/opengl/opengl_text.cpp
    ${EUI_ROOT}/core/window/window_backend.cpp
)

# Build shared library
add_library(eui-neo SHARED
    ${SHIM_SOURCES}
    ${ANDROID_SOURCES}
    ${EUI_CORE_SOURCES}
)

# Compile definitions
target_compile_definitions(eui-neo PRIVATE
    __ANDROID__
    GLFW_INCLUDE_NONE
    EUI_RENDER_BACKEND_OPENGL=1
)

# Force-include GLSL compatibility header
target_compile_options(eui-neo PRIVATE
    -include ${SHIM_ROOT}/eui_android_config.h
)

# Link Android system libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv3-lib GLESv3)

target_link_libraries(eui-neo
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${glesv3-lib}
)

# Compiler flags
target_compile_options(eui-neo PRIVATE
    -Os
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -Wall
    -Wextra
)
