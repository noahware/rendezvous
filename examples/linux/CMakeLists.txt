cmake_minimum_required(VERSION 3.16)
project(rendezvous_linux CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# rendezvous src root
set(RV_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

# library sources
file(GLOB_RECURSE RV_SOURCES
    "${RV_SRC_DIR}/render/*.cpp"
    "${RV_SRC_DIR}/gui/*.cpp"
    "${RV_SRC_DIR}/input/*.cpp"
)

# example sources
set(EXAMPLE_SOURCES main.cpp)

add_executable(rendezvous ${RV_SOURCES} ${EXAMPLE_SOURCES})

target_include_directories(rendezvous PRIVATE
    ${RV_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# find system packages
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype QUIET)

target_link_libraries(rendezvous PRIVATE
    OpenGL::GL
    OpenGL::GLX
    X11::X11
    ${CMAKE_DL_LIBS}
)

# spdlog — try pkg-config, then find_package, then header-only fallback
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SPDLOG QUIET spdlog)
endif()

if(SPDLOG_FOUND)
    target_include_directories(rendezvous PRIVATE ${SPDLOG_INCLUDE_DIRS})
    target_link_libraries(rendezvous PRIVATE ${SPDLOG_LIBRARIES})
else()
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        target_link_libraries(rendezvous PRIVATE spdlog::spdlog)
    else()
        message(WARNING "spdlog not found — install via: sudo apt install libspdlog-dev")
    endif()
endif()

# stb — check common paths
find_path(STB_INCLUDE_DIR stb_truetype.h
    PATHS /usr/include/stb /usr/local/include/stb /usr/include
)
if(STB_INCLUDE_DIR)
    target_include_directories(rendezvous PRIVATE ${STB_INCLUDE_DIR})
else()
    message(WARNING "stb headers not found — install via: sudo apt install libstb-dev")
endif()

# freetype (optional)
if(Freetype_FOUND)
    target_link_libraries(rendezvous PRIVATE Freetype::Freetype)
    target_compile_definitions(rendezvous PRIVATE RV_USE_FREETYPE)
endif()
