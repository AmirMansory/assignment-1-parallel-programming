cmake_minimum_required(VERSION 3.18)
project(CudaSimulation LANGUAGES CXX)

option(ENABLE_CUDA "Build with the CUDA backend" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_CUDA)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "The CUDA build requires CMake 3.24 or newer.")
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

find_package(SDL2 CONFIG QUIET)
find_package(SDL2_ttf CONFIG QUIET)

# Fallback to pkg-config when CMake config files are not provided by the
# distribution package. This makes the project work on systems that only
# install the classic pkg-config metadata (for example many Ubuntu/Debian
# packages).
if(NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PKG REQUIRED sdl2)
    set(SDL2_INCLUDE_DIRS ${SDL2_PKG_INCLUDE_DIRS})
    set(SDL2_LIBRARIES ${SDL2_PKG_LIBRARIES})
    # Use pkg-config results directly instead of fabricating an imported target.
    set(SDL2_USING_PKGCONFIG TRUE)
    set(SDL2_FOUND TRUE)
endif()

if(NOT SDL2_TTF_FOUND)
    if(NOT DEFINED SDL2_TTF_PKG)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2_TTF_PKG REQUIRED SDL2_ttf)
        set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_PKG_INCLUDE_DIRS})
        set(SDL2_TTF_LIBRARIES ${SDL2_TTF_PKG_LIBRARIES})
        # Use pkg-config results directly instead of an imported target.
        set(SDL2_TTF_USING_PKGCONFIG TRUE)
        set(SDL2_TTF_FOUND TRUE)
    endif()
endif()

set(SIMULATION_SOURCES
    main.cpp
    renderer.cpp
    simulation_cpu.cpp
)

if(ENABLE_CUDA)
    list(APPEND SIMULATION_SOURCES simulation_gpu.cu)
else()
    list(APPEND SIMULATION_SOURCES simulation_cuda_stub.cpp)
endif()

add_executable(simulation ${SIMULATION_SOURCES})
target_include_directories(simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(DEFINED SDL2_USING_PKGCONFIG)
    target_include_directories(simulation PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(simulation PRIVATE ${SDL2_LIBRARIES})
else()
    target_link_libraries(simulation PRIVATE SDL2::SDL2)
endif()

if(DEFINED SDL2_TTF_USING_PKGCONFIG)
    target_include_directories(simulation PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
    target_link_libraries(simulation PRIVATE ${SDL2_TTF_LIBRARIES})
else()
    target_link_libraries(simulation PRIVATE SDL2_ttf::SDL2_ttf)
endif()
if(WIN32)
    target_link_libraries(simulation PRIVATE SDL2::SDL2main)
endif()

if(ENABLE_CUDA)
    set_target_properties(simulation PROPERTIES CUDA_ARCHITECTURES native)
else()
    target_compile_definitions(simulation PRIVATE SIMULATION_CPU_ONLY=1)
endif()

message(STATUS "CUDA backend: ${ENABLE_CUDA}")
