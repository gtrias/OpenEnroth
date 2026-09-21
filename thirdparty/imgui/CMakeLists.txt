cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
# Create the imgui libraries here since the submodule doesn't contain a CMakeLists.txt

set(IMGUI_BACKENDS_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/)

function(copy_and_patch_imgui_file src_path dst_path)
    file(READ "${src_path}" file_contents)
    string(REPLACE "#include <GLES3/gl3.h>" "#include <glad/gl.h>" file_contents "${file_contents}")
    string(REPLACE "#include <GLES2/gl2.h>" "#include <glad/gl.h>" file_contents "${file_contents}")
    string(REPLACE "#include \"imgui.h\"" "#include <imgui/imgui.h>" file_contents "${file_contents}")
    file(WRITE "${dst_path}" "${file_contents}")
endfunction()

copy_and_patch_imgui_file(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h)
copy_and_patch_imgui_file(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp)
copy_and_patch_imgui_file(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.h ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.h)
copy_and_patch_imgui_file(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp)

set(IMGUI_BACKENDS_SOURCES
    ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp)

set(IMGUI_BACKENDS_HEADERS
    ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h
    ${IMGUI_BACKENDS_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.h)

add_library(imgui_backends STATIC ${IMGUI_BACKENDS_SOURCES} ${IMGUI_BACKENDS_HEADERS})
target_link_libraries(imgui_backends PRIVATE SDL3::SDL3 glad)
target_include_directories(imgui_backends PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)

if (OE_BUILD_PLATFORM STREQUAL "vita") # vita dlfcn (vita-rss-libdl) has no RTLD_NOLOAD.
    target_compile_definitions(imgui_backends PRIVATE RTLD_NOLOAD=0)

    # vitaGL is an ES 2.0 implementation and its GLSL translator only understands the legacy dialect, so ImGui must
    # take its "#version 100" ES2 shader path instead of the desktop "#version 130" default.
    target_compile_definitions(imgui_backends PRIVATE IMGUI_IMPL_OPENGL_ES2=1)
endif()

set(IMGUI_SOURCES
    imgui/imgui_draw.cpp
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp)

set(IMGUI_HEADERS
    imgui/imconfig.h
    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_link_libraries(imgui PUBLIC imgui_backends)
target_include_directories(imgui PUBLIC ${IMGUI_BACKENDS_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
