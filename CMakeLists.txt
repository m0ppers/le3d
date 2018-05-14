project(le3d CXX)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(LE3D_BUILD_EXAMPLES "Build the example programs" ON)
option(LE3D_AMMX "Enable Vampire AMMX optimizations on Amiga" Off)

SET(le3d_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_FILES
    engine/bitmap.cpp
    engine/bmpcache.cpp
    engine/bmpfile.cpp
    engine/bset.cpp
    engine/geometry.cpp
    engine/global.cpp
    engine/light.cpp
    engine/mesh.cpp
    engine/meshcache.cpp
    engine/objfile.cpp
    engine/rasterizer_float.cpp
    engine/rasterizer_integer.cpp
    engine/renderer.cpp
    engine/trilist.cpp
    engine/verlist.cpp
    tools/physics.cpp
)
set(LINK_LIBRARIES)

if (WIN32)
    list(APPEND LINK_LIBRARIES
        gdi32
        winmm
    )
    list(APPEND ENGINE_FILES
        engine/draw_win.cpp
        engine/gamepad_win.cpp
        engine/window_win.cpp
        tools/timing_win.cpp
    )
elseif (UNIX)
    find_package(X11 REQUIRED)
    list(APPEND LINK_LIBRARIES
        ${X11_LIBRARIES}
    )
    list(APPEND le3d_INCLUDE_DIRS
        ${X11_INCLUDE_DIR}
    )
    list(APPEND ENGINE_FILES
        engine/draw_unix.cpp
        engine/gamepad_unix.cpp
        engine/window_unix.cpp
        tools/timing_unix.cpp
    )
elseif (AMIGA)
    list(APPEND ENGINE_FILES
        engine/draw_amiga.cpp
        engine/window_amiga.cpp
        tools/timing_amiga.cpp
    )
    if(LE3D_AMMX)
        enable_language(ASM-VASM)
        add_library(ammx_support STATIC
            engine/ammx/ammx.s
        )
        list(APPEND LINK_LIBRARIES
            ammx_support
        )
    endif()
endif()

add_library(le3d
    ${ENGINE_FILES}
)

target_include_directories(
    le3d
    PUBLIC
    ${le3d_INCLUDE_DIRS}
)

target_link_libraries(
    le3d
    PUBLIC
    ${LINK_LIBRARIES}
)

if(AMIGA)
    if (LE3D_AMMX)
        target_compile_options(le3d PUBLIC -DAMMX=1)
    endif()
else()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set_target_properties(le3d PROPERTIES LINK_FLAGS "-m32")
        target_compile_options(le3d PUBLIC -m32) 
        target_compile_options(le3d PRIVATE -march=pentium4 -Wall -ffast-math -mfpmath=sse -fno-exceptions -fno-rtti -fno-stack-protector -fno-math-errno -fno-ident -ffunction-sections)
        if (WIN32)
            target_compile_options(le3d PRIVATE -D__MSVCRT_VERSION__=0x0700)
        endif()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(le3d PRIVATE -D__MSVCRT_VERSION__=0x0700 -D_CRT_SECURE_NO_WARNINGS=1 -D_USE_MATH_DEFINES=1)
        target_include_directories(le3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine/vs)
    endif()
endif()

if (LE3D_BUILD_EXAMPLES)
    add_subdirectory(examples)   
endif()
