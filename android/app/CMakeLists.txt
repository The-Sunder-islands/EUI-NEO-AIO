# CMakeLists.txt for EUI-NEO Android native library
cmake_minimum_required(VERSION 3.22.1)
project(eui-neo-native CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Render backend selection (set from Gradle via -DANDROID_STL=c++_static -DEUI_RENDER_BACKEND=vulkan)
set(EUI_RENDER_BACKEND "opengl" CACHE STRING "Render backend: opengl or vulkan")
set_property(CACHE EUI_RENDER_BACKEND PROPERTY STRINGS opengl vulkan)

# Paths
set(EUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SHIM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp)

# Include directories (shims first to override original GLFW/glad)
include_directories(
    ${SHIM_ROOT}/glfw_shim
    ${EUI_ROOT}
    ${EUI_ROOT}/include
    ${EUI_ROOT}/3rd
    ${EUI_ROOT}/3rd/freetype/include
    ${EUI_ROOT}/3rd/libpng-1.6.43
    ${EUI_ROOT}/3rd/zlib-1.3.1
)

if(EUI_RENDER_BACKEND STREQUAL "vulkan")
    include_directories(${SHIM_ROOT}/glad_gles)
endif()

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
    ${EUI_ROOT}/core/window/window_backend.cpp
)

if(EUI_RENDER_BACKEND STREQUAL "vulkan")
    list(APPEND EUI_CORE_SOURCES
        ${EUI_ROOT}/core/render/vulkan/vulkan_backend.cpp
        ${EUI_ROOT}/core/render/vulkan/vulkan_cache.cpp
        ${EUI_ROOT}/core/render/vulkan/vulkan_primitives.cpp
        ${EUI_ROOT}/core/render/vulkan/vulkan_text.cpp
        ${EUI_ROOT}/core/render/vulkan/vulkan_image.cpp
    )
else()
    list(APPEND EUI_CORE_SOURCES
        ${EUI_ROOT}/core/render/opengl/opengl_backend.cpp
        ${EUI_ROOT}/core/render/opengl/opengl_image.cpp
        ${EUI_ROOT}/core/render/opengl/opengl_primitives.cpp
        ${EUI_ROOT}/core/render/opengl/opengl_text.cpp
    )
endif()

# Minimal FreeType sources for Android build
set(FREETYPE_SOURCES
    ${EUI_ROOT}/3rd/freetype/src/base/ftbase.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftinit.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftsystem.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftdebug.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftbitmap.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftglyph.c
    ${EUI_ROOT}/3rd/freetype/src/base/ftmm.c
    ${EUI_ROOT}/3rd/freetype/src/smooth/smooth.c
    ${EUI_ROOT}/3rd/freetype/src/psnames/psnames.c
    ${EUI_ROOT}/3rd/freetype/src/sfnt/sfnt.c
    ${EUI_ROOT}/3rd/freetype/src/truetype/truetype.c
    ${EUI_ROOT}/3rd/freetype/src/autofit/autofit.c
    ${EUI_ROOT}/3rd/freetype/src/raster/raster.c
    ${EUI_ROOT}/3rd/freetype/src/cff/cff.c
    ${EUI_ROOT}/3rd/freetype/src/pshinter/pshinter.c
    ${EUI_ROOT}/3rd/freetype/src/psaux/psaux.c
    ${EUI_ROOT}/3rd/freetype/src/sdf/sdf.c
    ${EUI_ROOT}/3rd/freetype/src/svg/svg.c
    ${EUI_ROOT}/3rd/freetype/src/type1/type1.c
    ${EUI_ROOT}/3rd/freetype/src/cid/type1cid.c
    ${EUI_ROOT}/3rd/freetype/src/pfr/pfr.c
    ${EUI_ROOT}/3rd/freetype/src/type42/type42.c
    ${EUI_ROOT}/3rd/freetype/src/winfonts/winfnt.c
    ${EUI_ROOT}/3rd/freetype/src/pcf/pcf.c
    ${EUI_ROOT}/3rd/freetype/src/bdf/bdf.c
    ${EUI_ROOT}/3rd/freetype/src/gzip/ftgzip.c
    ${EUI_ROOT}/3rd/freetype/src/lzw/ftlzw.c
)
list(APPEND EUI_CORE_SOURCES ${FREETYPE_SOURCES})

# Minimal zlib sources (for freetype gzip support)
set(ZLIB_SOURCES
    ${EUI_ROOT}/3rd/zlib-1.3.1/adler32.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/crc32.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/inffast.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/inflate.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/inftrees.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/zutil.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/compress.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/uncompr.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/deflate.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/trees.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/gzclose.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/gzlib.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/gzread.c
    ${EUI_ROOT}/3rd/zlib-1.3.1/gzwrite.c
)
list(APPEND EUI_CORE_SOURCES ${ZLIB_SOURCES})

# Build shared library
add_library(eui-neo SHARED
    ${SHIM_SOURCES}
    ${ANDROID_SOURCES}
    ${EUI_CORE_SOURCES}
)

# Compile definitions (common)
target_compile_definitions(eui-neo PRIVATE
    __ANDROID__
    GLFW_INCLUDE_NONE
    FT2_BUILD_LIBRARY=1
    FT_CONFIG_OPTION_SYSTEM_ZLIB=0
)

if(EUI_RENDER_BACKEND STREQUAL "vulkan")
    target_compile_definitions(eui-neo PRIVATE
        EUI_RENDER_BACKEND_VULKAN=1
    )
else()
    target_compile_definitions(eui-neo PRIVATE
        EUI_RENDER_BACKEND_OPENGL=1
    )
    # Force-include GLSL compatibility header (only for OpenGL)
    target_compile_options(eui-neo PRIVATE
        -include ${SHIM_ROOT}/eui_android_config.h
    )
endif()

# Link system libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv3-lib GLESv3)

if(EUI_RENDER_BACKEND STREQUAL "vulkan")
    find_library(vulkan-lib vulkan)
    target_link_libraries(eui-neo
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${glesv3-lib}
        ${vulkan-lib}
    )
else()
    target_link_libraries(eui-neo
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${glesv3-lib}
    )
endif()

# Compiler flags
target_compile_options(eui-neo PRIVATE
    -Os
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -Wall
    -Wextra
)
