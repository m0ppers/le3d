if (WIN32)
    add_executable(cube
        cube/cube_win.cpp
    )
elseif(UNIX)
    add_executable(cube
        cube/cube_unix.cpp
    )
elseif(AMIGA)
    add_executable(cube
        cube/test_amiga.cpp
    )
endif()

target_link_libraries(
    cube
    PRIVATE
    le3d
)
target_include_directories(
    cube
    PRIVATE
    ${le3d_INCLUDE_DIRS}
)