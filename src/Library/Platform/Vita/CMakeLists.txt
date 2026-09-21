cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(LIBRARY_PLATFORM_VITA_SOURCES
        VitaMemory.cpp
        VitaOpenGLContext.cpp)

set(LIBRARY_PLATFORM_VITA_HEADERS
        VitaMemory.h
        VitaOpenGLContext.h)

add_library(library_platform_vita STATIC ${LIBRARY_PLATFORM_VITA_SOURCES} ${LIBRARY_PLATFORM_VITA_HEADERS})
target_check_style(library_platform_vita)

# vitaGL is the OpenGLES2 implementation, these are its dependencies as listed in the vitaGL samples.
target_link_libraries(library_platform_vita
        PUBLIC
        library_platform_interface
        utility
        vitaGL
        mathneon
        vitashark
        SceShaccCgExt
        SceShaccCg_stub
        SceGxm_stub
        SceDisplay_stub
        taihen_stub
        SceKernelDmacMgr_stub)
