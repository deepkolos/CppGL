cmake_minimum_required(VERSION 3.16)

project(CppGL CXX)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wno-unknown-attributes)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(BUILD_BENCHMARKS FLASE)
add_subdirectory(thirdparty/rttr-0.9.6)

add_executable(rainbow-triangle src/rainbow-triangle.cpp)
target_link_libraries(rainbow-triangle RTTR::Core ${OpenCV_LIBS})

add_executable(draw-cube-on-cube src/draw-cube-on-cube.cpp)
target_link_libraries(draw-cube-on-cube RTTR::Core ${OpenCV_LIBS})
