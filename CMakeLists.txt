cmake_minimum_required(VERSION 3.15)

project(anies-cli VERSION 0.5.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ctre CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(curlpp GIT_REPOSITORY https://github.com/jpbarrette/curlpp.git
                            GIT_TAG ec1b66e699557cd9d608d322c013a1ebda16bd08)
FetchContent_MakeAvailable(curlpp)

add_executable(${PROJECT_NAME} anies-cli.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE
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
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast
    -Wdouble-promotion
)

target_link_libraries(${PROJECT_NAME} PRIVATE curlpp ctre::ctre)
