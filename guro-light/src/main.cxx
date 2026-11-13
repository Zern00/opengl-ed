#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int WindowWidth = 1200;
constexpr int WindowHeight = 900;

struct Vertex {
    float position[3];
    float color[3];
    float normal[3];
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct Camera {
    glm::vec3 position = glm::vec3(3.0f, 2.4f, 4.0f);
    float yaw = -126.87f;
    float pitch = -20.78f;
    double lastCursorX = WindowWidth * 0.5;
    double lastCursorY = WindowHeight * 0.5;
    bool firstMouseEvent = true;
};

glm::vec3 CameraFront(const Camera& camera) {
    const float yaw = glm::radians(camera.yaw);
    const float pitch = glm::radians(camera.pitch);

    return glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)));
}

void CursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition) {
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera == nullptr) {
        return;
    }

    if (camera->firstMouseEvent) {
        camera->lastCursorX = xPosition;
        camera->lastCursorY = yPosition;
        camera->firstMouseEvent = false;
        return;
    }

    constexpr float sensitivity = 0.1f;
    const float xOffset = static_cast<float>(xPosition - camera->lastCursorX);
    const float yOffset = static_cast<float>(camera->lastCursorY - yPosition);
    camera->lastCursorX = xPosition;
    camera->lastCursorY = yPosition;

    camera->yaw += xOffset * sensitivity;
    camera->pitch += yOffset * sensitivity;
    camera->pitch = glm::clamp(camera->pitch, -89.0f, 89.0f);
}

void ProcessCameraInput(GLFWwindow* window, Camera& camera, float deltaTime) {
    const float speedMultiplier = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 3.0f : 1.0f;
    const float distance = 2.5f * speedMultiplier * deltaTime;
    const glm::vec3 front = CameraFront(camera);
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(front, worldUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.position += front * distance;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.position -= front * distance;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.position -= right * distance;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.position += right * distance;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.position -= worldUp * distance;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.position += worldUp * distance;
    }
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}", path.string()));
    }

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint CompileShader(GLenum type, const std::string& source) {
    const GLuint shader = glCreateShader(type);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        std::string log(static_cast<std::size_t>(logSize), '\0');
        glGetShaderInfoLog(shader, logSize, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::format("Shader compilation failed: {}", log));
    }

    return shader;
}

GLuint CreateProgram() {
    const GLuint vertexShader =
        CompileShader(GL_VERTEX_SHADER, ReadFile(std::filesystem::path(SHADER_DIR) / "simple.vert"));
    const GLuint fragmentShader =
        CompileShader(GL_FRAGMENT_SHADER, ReadFile(std::filesystem::path(SHADER_DIR) / "simple.frag"));

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logSize = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
        std::string log(static_cast<std::size_t>(logSize), '\0');
        glGetProgramInfoLog(program, logSize, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::format("Program linking failed: {}", log));
    }

    return program;
}

Mesh CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) {
    Mesh mesh;
    mesh.indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return mesh;
}

Mesh CreateGroundPlane() {
    constexpr float size = 5.0f;
    constexpr float y = 0.0f;

    const std::vector<Vertex> vertices = {
        {{-size, y, -size}, {0.55f, 0.57f, 0.60f}, {0.0f, 1.0f, 0.0f}},
        {{-size, y, size}, {0.55f, 0.57f, 0.60f}, {0.0f, 1.0f, 0.0f}},
        {{size, y, size}, {0.55f, 0.57f, 0.60f}, {0.0f, 1.0f, 0.0f}},
        {{size, y, -size}, {0.55f, 0.57f, 0.60f}, {0.0f, 1.0f, 0.0f}},
    };

    const std::vector<std::uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
    };

    return CreateMesh(vertices, indices);
}

Mesh CreateCylinder(std::uint32_t segmentCount) {
    constexpr float radius = 0.5f;
    constexpr float halfHeight = 0.5f;
    constexpr float red = 0.90f;
    constexpr float green = 0.1f;
    constexpr float blue = 0.1f;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    vertices.reserve((segmentCount + 1) * 4 + 2);
    indices.reserve(segmentCount * 12);

    for (std::uint32_t i = 0; i <= segmentCount; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segmentCount);
        const float x = std::cos(angle);
        const float z = std::sin(angle);

        vertices.push_back({{radius * x, -halfHeight, radius * z}, {red, green, blue}, {x, 0.0f, z}});
        vertices.push_back({{radius * x, halfHeight, radius * z}, {red, green, blue}, {x, 0.0f, z}});
    }

    for (std::uint32_t i = 0; i < segmentCount; ++i) {
        const std::uint32_t bottom = i * 2;
        const std::uint32_t top = bottom + 1;
        const std::uint32_t nextBottom = bottom + 2;
        const std::uint32_t nextTop = bottom + 3;

        indices.insert(indices.end(), {
                                          bottom,
                                          top,
                                          nextBottom,
                                          nextBottom,
                                          top,
                                          nextTop,
                                      });
    }

    const std::uint32_t topCenter = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back({{0.0f, halfHeight, 0.0f}, {red, green, blue}, {0.0f, 1.0f, 0.0f}});
    const std::uint32_t topRing = static_cast<std::uint32_t>(vertices.size());
    for (std::uint32_t i = 0; i <= segmentCount; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segmentCount);
        vertices.push_back(
            {{radius * std::cos(angle), halfHeight, radius * std::sin(angle)}, {red, green, blue}, {0.0f, 1.0f, 0.0f}});
    }

    const std::uint32_t bottomCenter = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back({{0.0f, -halfHeight, 0.0f}, {red, green, blue}, {0.0f, -1.0f, 0.0f}});
    const std::uint32_t bottomRing = static_cast<std::uint32_t>(vertices.size());
    for (std::uint32_t i = 0; i <= segmentCount; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segmentCount);
        vertices.push_back({{radius * std::cos(angle), -halfHeight, radius * std::sin(angle)},
                            {red, green, blue},
                            {0.0f, -1.0f, 0.0f}});
    }

    for (std::uint32_t i = 0; i < segmentCount; ++i) {
        indices.insert(indices.end(), {
                                          topCenter,
                                          topRing + i + 1,
                                          topRing + i,
                                          bottomCenter,
                                          bottomRing + i,
                                          bottomRing + i + 1,
                                      });
    }

    return CreateMesh(vertices, indices);
}

void DrawMesh(const Mesh& mesh) {
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
}

void DestroyMesh(const Mesh& mesh) {
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}

}

int main() {
    if (glfwInit() == GLFW_FALSE) {
        const char* description = nullptr;
        glfwGetError(&description);
        throw std::runtime_error(
            std::format("Failed to initialize GLFW: {}", description != nullptr ? description : "unknown error"));
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "Gouraud lighting", nullptr, nullptr);
    if (window == nullptr) {
        const char* description = nullptr;
        glfwGetError(&description);
        glfwTerminate();
        throw std::runtime_error(
            std::format("Failed to create a window: {}", description != nullptr ? description : "unknown error"));
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.035f, 0.045f, 0.060f, 1.0f);

    Camera camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    const GLuint program = CreateProgram();
    const Mesh groundPlane = CreateGroundPlane();
    const Mesh cylinder = CreateCylinder(64);

    const glm::vec3 lightPos = glm::vec3(1.5f, 2.0f, 1.5f);
    const glm::vec4 lightIntency = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    const glm::vec4 ambientIntensity = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    const float attenuationFactor = 0.03f;

    const GLint lightPosCameraLoc = glGetUniformLocation(program, "lightPosCamera");
    const GLint attenuationFactorLoc = glGetUniformLocation(program, "attenuationFactor");
    const GLint lightIntencyLoc = glGetUniformLocation(program, "lightIntency");
    const GLint ambientIntensityLoc = glGetUniformLocation(program, "ambientIntensity");
    const GLint modelToCameraLocation = glGetUniformLocation(program, "modelToCameraMatrix");
    const GLint cameraToClipLocation = glGetUniformLocation(program, "cameraToClipMatrix");

    const glm::mat4 groundModel(1.0f);
    const glm::mat4 cylinderModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));

    glUseProgram(program);
    glEnable(GL_MULTISAMPLE);
    float previousFrameTime = static_cast<float>(glfwGetTime());

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        const float currentFrameTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;
        ProcessCameraInput(window, camera, deltaTime);

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        if (framebufferWidth == 0 || framebufferHeight == 0) {
            continue;
        }

        glViewport(0, 0, framebufferWidth, framebufferHeight);

        glUniform4fv(lightIntencyLoc, 1, glm::value_ptr(lightIntency));
        glUniform4fv(ambientIntensityLoc, 1, glm::value_ptr(ambientIntensity));
        glUniform1f(attenuationFactorLoc, attenuationFactor);

        const glm::mat4 projection = glm::perspective(
            glm::radians(55.0f), static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight), 0.1f,
            100.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(cameraToClipLocation, 1, GL_FALSE, glm::value_ptr(projection));

        const glm::mat4 view =
            glm::lookAt(camera.position, camera.position + CameraFront(camera), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 lightPosCamera = glm::vec3(view * glm::vec4(lightPos, 1.0f));
        glUniform3fv(lightPosCameraLoc, 1, glm::value_ptr(lightPosCamera));

        const glm::mat4 groundModelToCamera = view * groundModel;
        glUniformMatrix4fv(modelToCameraLocation, 1, GL_FALSE, glm::value_ptr(groundModelToCamera));
        DrawMesh(groundPlane);

        const glm::mat4 cylinderModelToCamera = view * cylinderModel;
        glUniformMatrix4fv(modelToCameraLocation, 1, GL_FALSE, glm::value_ptr(cylinderModelToCamera));
        DrawMesh(cylinder);

        glfwSwapBuffers(window);
    }

    glUseProgram(0);
    DestroyMesh(cylinder);
    DestroyMesh(groundPlane);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
}
