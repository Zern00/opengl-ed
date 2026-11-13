cmake_minimum_required(VERSION 3.20)
project(blinn-phong-light LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(blinn-phong-light src/glad.c src/main.cxx)
target_include_directories(blinn-phong-light PRIVATE include)
target_compile_definitions(blinn-phong-light PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
target_link_libraries(blinn-phong-light PRIVATE glfw OpenGL::GL glm::glm ${CMAKE_DL_LIBS})
