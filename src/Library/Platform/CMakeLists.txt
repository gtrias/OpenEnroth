cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_subdirectory(Application)
add_subdirectory(Filters)
add_subdirectory(Interface)
add_subdirectory(Null)
add_subdirectory(Proxy)
add_subdirectory(Sdl)

if(OE_BUILD_PLATFORM STREQUAL "vita")
    add_subdirectory(Vita)
endif()
