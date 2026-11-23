cmake_minimum_required(VERSION 3.15)

project(anies-cli VERSION 1.0.7 LANGUAGES CXX)

add_compile_options(
    "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=."
    "-ffile-prefix-map=${CMAKE_BINARY_DIR}=."
    "-fmacro-prefix-map=${VCPKG_INSTALLED_DIR}=."
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
set(CMAKE_BUILD_TYPE Release)

set(ADDITIONAL_LIBS)

find_package(ctre CONFIG REQUIRED)
find_package(botan CONFIG REQUIRED)
if(NOT ANDROID)
    find_package(Boost REQUIRED COMPONENTS process)
endif()
find_package(ftxui CONFIG REQUIRED)
if(WIN32 OR ANDROID)
    find_package(CURL REQUIRED)
    set(ADDITIONAL_LIBS CURL::libcurl)
endif()

include(FetchContent)
FetchContent_Declare(
    curlpp
    GIT_REPOSITORY https://github.com/jpbarrette/curlpp.git
    GIT_TAG ec1b66e699557cd9d608d322c013a1ebda16bd08
)

set(CURLPP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(curlpp)

set(PROJECT_LIBS
    curlpp_static
    ctre::ctre
    Botan::Botan-static
    botan::botan-static 
    ftxui::ftxui
    ${ADDITIONAL_LIBS}
)

if(NOT ANDROID)
    list(APPEND PROJECT_LIBS
        Boost::process
    )
endif()

set(COMPILER_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wsign-conversion
    -Wmisleading-indentation
    -Wnull-dereference
    -Wdouble-promotion
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND COMPILER_FLAGS
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )
endif()

add_executable(${PROJECT_NAME} anies-cli.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMPILER_FLAGS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_LIBS}
)

if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()
