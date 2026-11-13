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
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct Camera {
    glm::vec3 position{3.0f, 2.4f, 4.0f};
    float yaw = -126.87f;
    float pitch = -20.78f;
    double lastX = WindowWidth * 0.5;
    double lastY = WindowHeight * 0.5;
    bool firstMouse = true;
};

struct alignas(16) MaterialBlock {
    glm::vec4 diffuseColor;
    glm::vec4 specularColor;
    float specularShininess;
    float padding[3];
};
static_assert(sizeof(MaterialBlock) == 48);

glm::vec3 CameraFront(const Camera& camera) {
    const float yaw = glm::radians(camera.yaw);
    const float pitch = glm::radians(camera.pitch);
    return glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)));
}

void CursorCallback(GLFWwindow* window, double x, double y) {
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera == nullptr) {
        return;
    }
    if (camera->firstMouse) {
        camera->lastX = x;
        camera->lastY = y;
        camera->firstMouse = false;
        return;
    }
    constexpr float sensitivity = 0.1f;
    camera->yaw += static_cast<float>(x - camera->lastX) * sensitivity;
    camera->pitch += static_cast<float>(camera->lastY - y) * sensitivity;
    camera->pitch = glm::clamp(camera->pitch, -89.0f, 89.0f);
    camera->lastX = x;
    camera->lastY = y;
}

void ProcessInput(GLFWwindow* window, Camera& camera, float deltaTime, GLuint nearestSampler, GLuint linearSampler) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    const float boost = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 3.0f : 1.0f;
    const float step = 2.5f * boost * deltaTime;
    const glm::vec3 front = CameraFront(camera);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(front, up));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.position += front * step;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.position -= front * step;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.position -= right * step;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.position += right * step;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.position -= up * step;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.position += up * step;
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        glBindSampler(0, nearestSampler);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        glBindSampler(0, linearSampler);
    }
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error(std::format("Failed to open {}", path.string()));
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint CompileShader(GLenum type, const std::filesystem::path& path) {
    const std::string source = ReadFile(path);
    const char* pointer = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &pointer, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(length), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::format("{}: {}", path.string(), log));
    }
    return shader;
}

GLuint CreateProgram() {
    const auto shaderDirectory = std::filesystem::path(SHADER_DIR);
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, shaderDirectory / "scene.vert");
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, shaderDirectory / "scene.frag");
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(length), '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::format("Program link failed: {}", log));
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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return mesh;
}

Mesh CreateGround() {
    constexpr float s = 5.0f;
    return CreateMesh(
        {
            {{-s, 0.0f, -s}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-s, 0.0f, s}, {0.0f, 1.0f, 0.0f}, {0.0f, 16.0f}},
            {{s, 0.0f, s}, {0.0f, 1.0f, 0.0f}, {16.0f, 16.0f}},
            {{s, 0.0f, -s}, {0.0f, 1.0f, 0.0f}, {16.0f, 0.0f}},
        },
        {0, 1, 2, 2, 3, 0});
}

Mesh CreateCylinder(std::uint32_t segments) {
    constexpr float radius = 0.6f;
    constexpr float halfHeight = 0.75f;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    for (std::uint32_t i = 0; i <= segments; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * i / segments;
        const float x = std::cos(angle);
        const float z = std::sin(angle);
        const float u = static_cast<float>(i) / static_cast<float>(segments);
        vertices.push_back({{radius * x, -halfHeight, radius * z}, {x, 0.0f, z}, {u, 0.0f}});
        vertices.push_back({{radius * x, halfHeight, radius * z}, {x, 0.0f, z}, {u, 1.0f}});
    }
    for (std::uint32_t i = 0; i < segments; ++i) {
        const std::uint32_t b = 2 * i;
        indices.insert(indices.end(), {b, b + 1, b + 2, b + 2, b + 1, b + 3});
    }
    const std::uint32_t topCenter = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back({{0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}});
    const std::uint32_t topRing = static_cast<std::uint32_t>(vertices.size());
    for (std::uint32_t i = 0; i <= segments; ++i) {
        const float a = 2.0f * std::numbers::pi_v<float> * i / segments;
        const float x = std::cos(a);
        const float z = std::sin(a);
        vertices.push_back(
            {{radius * x, halfHeight, radius * z}, {0.0f, 1.0f, 0.0f}, {0.5f + 0.5f * x, 0.5f + 0.5f * z}});
    }
    const std::uint32_t bottomCenter = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}});
    const std::uint32_t bottomRing = static_cast<std::uint32_t>(vertices.size());
    for (std::uint32_t i = 0; i <= segments; ++i) {
        const float a = 2.0f * std::numbers::pi_v<float> * i / segments;
        const float x = std::cos(a);
        const float z = std::sin(a);
        vertices.push_back(
            {{radius * x, -halfHeight, radius * z}, {0.0f, -1.0f, 0.0f}, {0.5f + 0.5f * x, 0.5f - 0.5f * z}});
    }
    for (std::uint32_t i = 0; i < segments; ++i) {
        indices.insert(indices.end(),
                       {topCenter, topRing + i + 1, topRing + i, bottomCenter, bottomRing + i, bottomRing + i + 1});
    }
    return CreateMesh(vertices, indices);
}

GLuint CreateMaterialBuffer(const MaterialBlock& material) {
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(material), &material, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return buffer;
}

void Draw(const Mesh& mesh, GLuint materialBuffer) {
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, materialBuffer);
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
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window =
        glfwCreateWindow(WindowWidth, WindowHeight, "Exercise 15.1 - Texture filtering", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to load OpenGL");
    }

    Camera camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, CursorCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.025f, 0.035f, 0.055f, 1.0f);

    const GLuint program = CreateProgram();
    std::vector<GLubyte> textData;
    GLuint width = 64;
    GLuint height = 64;
    textData.resize(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (y * width + x);

            int cellX = x / 8;
            int cellY = y / 8;

            bool isBright = (cellX + cellY) % 2;

            textData[idx] = isBright * 255.0f;
        }
    }

    GLuint textureUnit;
    glGenTextures(1, &textureUnit);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureUnit);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, &textData[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    GLuint textureSamplers[2]{};
    glGenSamplers(2, textureSamplers);
    const GLuint nearestSampler = textureSamplers[0];
    const GLuint linearSampler = textureSamplers[1];

    for (const GLuint sampler : textureSamplers) {
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glSamplerParameteri(nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(linearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(linearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindSampler(0, nearestSampler);

    const GLuint materialIndex = glGetUniformBlockIndex(program, "Material");
    if (materialIndex == GL_INVALID_INDEX) {
        throw std::runtime_error("Material UBO was not found");
    }
    glUniformBlockBinding(program, materialIndex, 0);

    const Mesh ground = CreateGround();
    const Mesh cylinder = CreateCylinder(64);
    const MaterialBlock groundMaterial{
        {0.28f, 0.32f, 0.38f, 1.0f}, {0.10f, 0.10f, 0.10f, 1.0f}, 16.0f, {0.0f, 0.0f, 0.0f}};
    const MaterialBlock cylinderMaterial{
        {0.72f, 0.12f, 0.07f, 1.0f}, {0.95f, 0.90f, 0.78f, 1.0f}, 64.0f, {0.0f, 0.0f, 0.0f}};
    const GLuint groundMaterialBuffer = CreateMaterialBuffer(groundMaterial);
    const GLuint cylinderMaterialBuffer = CreateMaterialBuffer(cylinderMaterial);

    const GLint isTextLoc = glGetUniformLocation(program, "isText");
    const GLint samplerLoc = glGetUniformLocation(program, "textureSampler");
    const GLint modelToCameraLoc = glGetUniformLocation(program, "modelToCameraMatrix");
    const GLint normalToCameraLoc = glGetUniformLocation(program, "normalModelToCameraMatrix");
    const GLint cameraToClipLoc = glGetUniformLocation(program, "cameraToClipMatrix");
    const GLint lightPositionLoc = glGetUniformLocation(program, "lightPositionCamera");
    const GLint lightIntensityLoc = glGetUniformLocation(program, "lightIntensity");
    const GLint ambientLoc = glGetUniformLocation(program, "ambientIntensity");
    const GLint attenuationLoc = glGetUniformLocation(program, "attenuationFactor");

    const glm::mat4 groundModel(1.0f);
    const glm::mat4 cylinderModel = glm::translate(glm::mat4(1.0f), {0.0f, 0.75f, 0.0f});
    const glm::vec3 lightWorld(1.8f, 2.8f, 1.6f);
    float previousTime = static_cast<float>(glfwGetTime());

    glUseProgram(program);
    glUniform1i(samplerLoc, 0);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const float now = static_cast<float>(glfwGetTime());
        ProcessInput(window, camera, now - previousTime, nearestSampler, linearSampler);
        previousTime = now;

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            continue;
        }
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 view = glm::lookAt(camera.position, camera.position + CameraFront(camera), {0.0f, 1.0f, 0.0f});
        const glm::mat4 projection =
            glm::perspective(glm::radians(55.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
        const glm::vec3 lightCamera = glm::vec3(view * glm::vec4(lightWorld, 1.0f));
        glUniformMatrix4fv(cameraToClipLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(lightPositionLoc, 1, glm::value_ptr(lightCamera));
        glUniform3f(lightIntensityLoc, 1.35f, 1.25f, 1.10f);
        glUniform3f(ambientLoc, 0.08f, 0.09f, 0.12f);
        glUniform1f(attenuationLoc, 0.045f);

        const auto drawObject = [&](const Mesh& mesh, const glm::mat4& model, GLuint material, bool useTexture) {
            const glm::mat4 modelToCamera = view * model;
            const glm::mat3 normalToCamera = glm::transpose(glm::inverse(glm::mat3(modelToCamera)));
            glUniformMatrix4fv(modelToCameraLoc, 1, GL_FALSE, glm::value_ptr(modelToCamera));
            glUniformMatrix3fv(normalToCameraLoc, 1, GL_FALSE, glm::value_ptr(normalToCamera));
            glUniform1i(isTextLoc, useTexture ? 1 : 0);
            Draw(mesh, material);
        };
        drawObject(ground, groundModel, groundMaterialBuffer, true);
        drawObject(cylinder, cylinderModel, cylinderMaterialBuffer, false);
        glfwSwapBuffers(window);
    }
    glDeleteSamplers(2, textureSamplers);
    glDeleteTextures(1, &textureUnit);
    glDeleteBuffers(1, &cylinderMaterialBuffer);
    glDeleteBuffers(1, &groundMaterialBuffer);
    DestroyMesh(cylinder);
    DestroyMesh(ground);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
}
