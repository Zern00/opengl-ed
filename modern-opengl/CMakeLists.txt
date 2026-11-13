cmake_minimum_required(VERSION 3.20)
project(modern-opengl LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(ssbo-compute src/glad.c src/ssbo-compute.cxx src/common.cxx)
target_include_directories(ssbo-compute PRIVATE include)
target_compile_definitions(ssbo-compute PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
target_link_libraries(ssbo-compute PRIVATE glfw OpenGL::GL glm::glm ${CMAKE_DL_LIBS})
