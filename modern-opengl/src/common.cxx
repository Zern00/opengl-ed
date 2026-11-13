#include "common.hxx"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

int debugErrors = 0;

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + path.string());
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint CompileShader(GLenum type, const std::filesystem::path& path) {
    const std::string source = ReadFile(path);
    const char* src = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(std::max(length, 1), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(path.string() + ": " + log);
    }
    return shader;
}

GLuint LinkProgram(std::initializer_list<std::pair<GLenum, std::filesystem::path>> stages) {
    std::vector<GLuint> shaders;
    GLuint program = 0;
    try {
        for (const auto& [type, path] : stages) {
            shaders.push_back(CompileShader(type, path));
        }
        program = glCreateProgram();
        for (GLuint shader : shaders) {
            glAttachShader(program, shader);
        }
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(std::max(length, 1), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            throw std::runtime_error("Shader link error: " + log);
        }
    } catch (...) {
        if (program) {
            glDeleteProgram(program);
        }
        for (GLuint shader : shaders) {
            glDeleteShader(shader);
        }
        throw;
    }
    for (GLuint shader : shaders) {
        glDetachShader(program, shader);
        glDeleteShader(shader);
    }
    return program;
}

}

GLuint CreateProgram(const char* vertex, const char* fragment) {
    const std::filesystem::path directory = SHADER_DIR;
    return LinkProgram({{GL_VERTEX_SHADER, directory / vertex}, {GL_FRAGMENT_SHADER, directory / fragment}});
}

GLuint CreateComputeProgram(const char* compute) {
    return LinkProgram({{GL_COMPUTE_SHADER, std::filesystem::path(SHADER_DIR) / compute}});
}

namespace {

void APIENTRY DebugCallback(GLenum, GLenum type, GLuint, GLenum, GLsizei, const GLchar* message, const void*) {
    if (type == GL_DEBUG_TYPE_ERROR) {
        ++debugErrors;
    }
    std::cerr << "OpenGL debug: " << message << '\n';
}

}

GLFWwindow* CreateWindow(const char* title) {
    glfwSetErrorCallback([](int, const char* description) {
        std::cerr << "GLFW: " << description << '\n';
    });
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create an OpenGL 4.5 window");
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) || GLVersion.major < 4 ||
        (GLVersion.major == 4 && GLVersion.minor < 5)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Required OpenGL version is unavailable");
    }
    glfwSwapInterval(1);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    std::cout << title << " | OpenGL " << glGetString(GL_VERSION) << '\n';
    return window;
}

void DestroyWindow(GLFWwindow* window) {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool BeginFrame(GLFWwindow* window, int& width, int& height) {
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        glfwWaitEventsTimeout(0.05);
        return false;
    }
    glViewport(0, 0, width, height);
    return !glfwWindowShouldClose(window);
}

bool HasDebugErrors() {
    return debugErrors != 0;
}

Options ReadOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument != "--frames" || i + 1 == argc) {
            throw std::runtime_error("Usage: ssbo-compute [--frames N]");
        }
        const std::string_view value = argv[++i];
        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), options.frames);
        if (error != std::errc{} || end != value.data() + value.size() || options.frames <= 0) {
            throw std::runtime_error("--frames requires a positive integer");
        }
    }
    return options;
}
