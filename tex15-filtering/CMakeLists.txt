cmake_minimum_required(VERSION 3.20)
project(tex15-filtering LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(tex15-filtering src/glad.c src/main.cxx)
target_include_directories(tex15-filtering PRIVATE include)
target_compile_definitions(tex15-filtering PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
target_link_libraries(tex15-filtering PRIVATE glfw OpenGL::GL glm::glm ${CMAKE_DL_LIBS})
