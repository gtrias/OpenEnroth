cmake_minimum_required(VERSION 3.20.4 FATAL_ERROR)

project(glad)

if(OE_BUILD_PLATFORM STREQUAL "vita")
    set(GLAD_GL_SOURCES src/gl.c) # No EGL on Vita, loading goes through SDL/vitaGL.
else()
    set(GLAD_GL_SOURCES src/gl.c
                        src/egl.c)
endif()

set(GLAD_GL_HEADERS include/glad/gl.h
                    include/KHR/khrplatform.h)
if(NOT OE_BUILD_PLATFORM STREQUAL "vita")
    list(APPEND GLAD_GL_HEADERS include/glad/egl.h include/EGL/eglplatform.h)
endif()

add_library(glad STATIC ${GLAD_GL_SOURCES} ${GLAD_GL_HEADERS})
target_include_directories(glad PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

source_check_style(glad "${GLAD_GL_SOURCES}")
