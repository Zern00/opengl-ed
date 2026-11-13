#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <format>
#include <string>
#include <filesystem>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

struct Camera {
    glm::vec3 pos;
    glm::vec3 view;
    glm::vec3 up;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
};

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}", path.string()));
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint CompileShader(GLenum type, const std::filesystem::path& path) {
    const std::string source = ReadFile(path);
    const char* src = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::format("{}: {}", path.string(), log));
    }
    return shader;
}

void APIENTRY DebugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* message, const void*) {
    std::cerr << "OpenGL debug: " << message << '\n';
}

GLFWwindow* CreateWindow() {
    if (!glfwInit()) {
        const char* description;
        glfwGetError(&description);
        throw std::runtime_error(std::format("glfwInit error: {}", description));
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "instance", nullptr, nullptr);
    if (!window) {
        const char* description;
        glfwGetError(&description);
        throw std::runtime_error(std::format("glfwCreateWindow error: {}", description));
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("gladLoadGLLoader error");
    }

    return window;
}

void InitializeDebugOutput() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
}

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
};

Mesh CreateCube() {
    Mesh mesh;
    const Vertex cube[] = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},

        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},

        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},

        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    };

    const GLuint cubeIndices[] = {
        0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
        12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
    };

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glObjectLabel(GL_BUFFER, mesh.vbo, -1, "Cube vertex buffer");
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), &cube, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glObjectLabel(GL_BUFFER, mesh.ebo, -1, "Cube element buffer");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), &cubeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);

    return mesh;
}

GLuint CreateProgram() {
    const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, std::filesystem::path(SHADER_DIR) / "cube.vert");
    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, std::filesystem::path(SHADER_DIR) / "cube.frag");
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::format("Shader link error: {}", log));
    }
    return program;
}

void UpdateInstances(glm::mat4* transforms, int count, float dt) {
    for (int i = 0; i < count; ++i) {
        const float speed = 0.5f + static_cast<float>(i % 11) * 0.07f;
        transforms[i] = glm::rotate(transforms[i], speed * dt, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}

int main() {
    GLFWwindow* window = CreateWindow();
    InitializeDebugOutput();
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Initialization pass");
    const Mesh mesh = CreateCube();
    glEnable(GL_DEPTH_TEST);
    const GLuint program = CreateProgram();
    const GLint viewProjectionLocation = glGetUniformLocation(program, "viewProjection");
    const GLint fragmentWorkLocation = glGetUniformLocation(program, "fragmentWorkIterations");
    constexpr int fragmentWorkIterations = 0;
    glUseProgram(program);
    glUniform1i(fragmentWorkLocation, fragmentWorkIterations);

    constexpr int columns = 40;
    constexpr int rows = 25;
    constexpr int cubeCount = columns * rows;
    constexpr float spacing = 2.0f;
    const float fov = glm::radians(50.0f);
    double fpsStart = glfwGetTime();
    unsigned int frameCount = 0;

    glm::mat4 instancePositions[cubeCount];
    for (int i = 0; i < cubeCount; ++i) {
        const int column = i % columns;
        const int row = i / columns;
        const glm::vec3 position{(column - (columns - 1) * 0.5f) * spacing, (row - (rows - 1) * 0.5f) * spacing, 0.0f};
        instancePositions[i] = glm::translate(glm::mat4(1.0f), position);
    }

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glObjectLabel(GL_BUFFER, instanceVBO, -1, "Cube instance matrices");
    glBufferData(GL_ARRAY_BUFFER, sizeof(instancePositions), &instancePositions, GL_DYNAMIC_DRAW);

    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              reinterpret_cast<void*>(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(3 + i);
        glVertexAttribDivisor(3 + i, 1);
    }

    glm::vec3 instanceColors[cubeCount];
    for (int i = 0; i < cubeCount; ++i) {
        const int column = i % columns;
        const int row = i / columns;
        instanceColors[i] = glm::vec3{0.3f + 0.6f * static_cast<float>(column) / (columns - 1),
                                      0.35f + 0.55f * static_cast<float>(row) / (rows - 1), 0.85f};
    }

    GLuint instanceVBOColor;
    glGenBuffers(1, &instanceVBOColor);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBOColor);
    glObjectLabel(GL_BUFFER, instanceVBOColor, -1, "Cube instance colors");
    glBufferData(GL_ARRAY_BUFFER, sizeof(instanceColors), instanceColors, GL_STATIC_DRAW);

    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);
    glfwSwapInterval(0);
    float prevTime = static_cast<float>(glfwGetTime());
    GLuint startQuery, endQuery, samplesPassedQuery;
    glGenQueries(1, &startQuery);
    glGenQueries(1, &endQuery);
    glGenQueries(1, &samplesPassedQuery);
    glPopDebugGroup();
    while (!glfwWindowShouldClose(window)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Main render pass");
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glPopDebugGroup();
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        glViewport(0, 0, width, height);
        glClearColor(0.025f, 0.035f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(width) / height;
        const float halfWidth = (columns - 1) * spacing * 0.5f + 2.0f;
        const float halfHeight = (rows - 1) * spacing * 0.5f + 2.0f;
        const float distance = std::max(halfHeight, halfWidth / aspect) / std::tan(fov * 0.5f) + 2.0f;
        const Camera camera{{0.0f, 0.0f, distance}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        const glm::mat4 view = glm::lookAt(camera.pos, camera.view, camera.up);
        const glm::mat4 projection = glm::perspective(fov, aspect, 0.1f, distance + 10.0f);
        const glm::mat4 viewProjection = projection * view;

        glUseProgram(program);
        glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
        glBindVertexArray(mesh.vao);
        const float curTime = static_cast<float>(glfwGetTime());
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        UpdateInstances(instancePositions, cubeCount, curTime - prevTime);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(instancePositions), &instancePositions);
        glBeginQuery(GL_SAMPLES_PASSED, samplesPassedQuery);
        glQueryCounter(startQuery, GL_TIMESTAMP);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, cubeCount);
        glQueryCounter(endQuery, GL_TIMESTAMP);
        glEndQuery(GL_SAMPLES_PASSED);

        glfwSwapBuffers(window);

        GLint64 startTime, endTime, samplesPassed;
        glGetQueryObjecti64v(startQuery, GL_QUERY_RESULT, &startTime);
        glGetQueryObjecti64v(endQuery, GL_QUERY_RESULT, &endTime);
        glGetQueryObjecti64v(samplesPassedQuery, GL_QUERY_RESULT, &samplesPassed);

        const std::string log = std::format("{} ms passed by gpu draw, passed {} samples",
                                            (endTime - startTime) / 1'000'000.0f, samplesPassed);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 0, GL_DEBUG_SEVERITY_LOW, -1,
                             log.c_str());
        ++frameCount;
        const double now = glfwGetTime();
        const double elapsed = now - fpsStart;
        if (elapsed >= 1.0) {
            std::cout << std::format("FPS: {:.1f}\n", frameCount / elapsed) << std::flush;
            frameCount = 0;
            fpsStart = now;
        }
        prevTime = curTime;
        glPopDebugGroup();
    }

    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteBuffers(1, &instanceVBOColor);
    glDeleteQueries(1, &startQuery);
    glDeleteQueries(1, &endQuery);
    glDeleteQueries(1, &samplesPassedQuery);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
}
