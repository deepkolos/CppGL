cmake_minimum_required(VERSION 3.16)

project(CppGL CXX)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wno-unknown-attributes)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(BUILD_BENCHMARKS OFF)
set(BUILD_UNIT_TESTS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_DOCUMENTATION OFF)
set(BUILD_PACKAGE OFF)
set(BUILD_INSTALLER OFF)
set(USE_PCH OFF)
add_subdirectory(thirdparty/rttr-0.9.6)

include_directories(thirdparty/tinygltf)

add_library(CppGL STATIC src/api.cpp 
                         src/helper.cpp 
                         src/math.cpp 
                         src/global.cpp 
                         src/shader.cpp)
target_include_directories(CppGL PUBLIC includes)
target_link_libraries(CppGL RTTR::Core)

add_executable(rainbow-triangle examples/rainbow-triangle.cpp)
target_link_libraries(rainbow-triangle RTTR::Core ${OpenCV_LIBS} CppGL)

add_executable(texture-triangle examples/texture-triangle.cpp)
target_link_libraries(texture-triangle RTTR::Core ${OpenCV_LIBS} CppGL)

add_executable(draw-cubes examples/draw-cubes.cpp)
target_link_libraries(draw-cubes RTTR::Core ${OpenCV_LIBS} CppGL)

add_executable(draw-cube-on-cube examples/draw-cube-on-cube.cpp)
target_link_libraries(draw-cube-on-cube RTTR::Core ${OpenCV_LIBS} CppGL)

add_executable(draw-gltf examples/draw-gltf.cpp)
target_link_libraries(draw-gltf RTTR::Core ${OpenCV_LIBS} CppGL)
