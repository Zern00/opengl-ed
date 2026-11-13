#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
constexpr GLsizei PARTICLE_COUNT = 2048;

struct Camera {
    glm::vec3 pos{0.0f, 4.0f, 7.0f};
    float yaw = -90.0f;
    float pitch = -30.0f;
    bool looking = false;
    double lastX = 0.0;
    double lastY = 0.0;

    glm::vec3 Forward() const {
        const float y = glm::radians(yaw);
        const float p = glm::radians(pitch);
        return {std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)};
    }

    void Update(GLFWwindow* window, float dt) {
        const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
        const bool mouseDown = focused && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (mouseDown != looking) {
            looking = mouseDown;
            glfwSetInputMode(window, GLFW_CURSOR, looking ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        if (!focused) {
            return;
        }

        if (looking) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            yaw += static_cast<float>(x - lastX) * 0.1f;
            pitch = std::clamp(pitch + static_cast<float>(lastY - y) * 0.1f, -89.0f, 89.0f);
            lastX = x;
            lastY = y;
        }

        const glm::vec3 up{0.0f, 1.0f, 0.0f};
        const glm::vec3 forward = Forward();
        const glm::vec3 right = glm::normalize(glm::cross(forward, up));
        glm::vec3 movement{0.0f};
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            movement += forward;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            movement -= forward;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            movement += right;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            movement -= right;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            movement += up;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            movement -= up;
        }
        const float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 9.0f : 3.0f;
        if (glm::dot(movement, movement) > 0.0f) {
            pos += glm::normalize(movement) * speed * dt;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float age;
    float lifetime;
};
static_assert(sizeof(Particle) == 8 * sizeof(float));

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

GLuint CreateProgram(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath,
                     const std::filesystem::path& tessellationControlShaderPath,
                     const std::filesystem::path& tessellationEvaluationShaderPath) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Program init");
    try {
        const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderPath);
        GLuint fragmentShader;
        GLuint tessellationControlShader;
        GLuint tessellationEvaluationShader;
        try {
            fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderPath);
            try {
                tessellationControlShader = CompileShader(GL_TESS_CONTROL_SHADER, tessellationControlShaderPath);
                try {
                    tessellationEvaluationShader =
                        CompileShader(GL_TESS_EVALUATION_SHADER, tessellationEvaluationShaderPath);
                } catch (...) {
                    glDeleteShader(tessellationControlShader);
                    throw;
                }
            } catch (...) {
                glDeleteShader(fragmentShader);
                throw;
            }
        } catch (...) {
            glDeleteShader(vertexShader);
            throw;
        }
        const GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glAttachShader(program, tessellationControlShader);
        glAttachShader(program, tessellationEvaluationShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteShader(tessellationControlShader);
        glDeleteShader(tessellationEvaluationShader);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(std::max(length, 1), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("Shader link error: " + log);
        }
        glPopDebugGroup();
        return program;
    } catch (...) {
        glPopDebugGroup();
        throw;
    }
}

GLuint CreateParticleProgram(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath = {}) {
    const bool update = fragmentPath.empty();
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1,
                     update ? "Particles update program" : "Particles render program");
    GLuint vertex = 0, fragment = 0, program = 0;
    try {
        vertex = CompileShader(GL_VERTEX_SHADER, vertexPath);
        if (!update) {
            fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentPath);
        }
        program = glCreateProgram();
        glAttachShader(program, vertex);
        if (fragment) {
            glAttachShader(program, fragment);
        }
        if (update) {
            const char* varyings[] = {"tfPosition", "tfVelocity", "tfAge", "tfLifetime"};
            glTransformFeedbackVaryings(program, 4, varyings, GL_INTERLEAVED_ATTRIBS);
        }
        glLinkProgram(program);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(std::max(length, 1), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            throw std::runtime_error("Particle shader link error: " + log);
        }
        glObjectLabel(GL_PROGRAM, program, -1, update ? "Particles update" : "Particles render");
    } catch (...) {
        if (program) {
            glDeleteProgram(program);
        }
        if (fragment) {
            glDeleteShader(fragment);
        }
        if (vertex) {
            glDeleteShader(vertex);
        }
        glPopDebugGroup();
        throw;
    }
    glDeleteShader(vertex);
    if (fragment) {
        glDeleteShader(fragment);
    }
    glPopDebugGroup();
    return program;
}

void APIENTRY DebugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* message, const void*) {
    std::cerr << "OpenGL debug: " << message << '\n';
}

int main() {
    glfwSetErrorCallback([](int, const char* description) {
        std::cerr << "GLFW: " << description << '\n';
    });
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "core4", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to load OpenGL functions\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSwapInterval(0);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Initialization pass");
    glEnable(GL_DEPTH_TEST);

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    GLuint program = 0, particleUpdateProgram = 0, particleRenderProgram = 0;
    try {
        program = CreateProgram(
            std::filesystem::path(SHADER_DIR) / "plane.vert", std::filesystem::path(SHADER_DIR) / "plane.frag",
            std::filesystem::path(SHADER_DIR) / "plane.tesc", std::filesystem::path(SHADER_DIR) / "plane.tese");
        particleUpdateProgram = CreateParticleProgram(std::filesystem::path(SHADER_DIR) / "particles_update.vert");
        particleRenderProgram = CreateParticleProgram(std::filesystem::path(SHADER_DIR) / "particles_render.vert",
                                                      std::filesystem::path(SHADER_DIR) / "particles_render.frag");
    } catch (const std::exception& error) {
        if (particleRenderProgram) {
            glDeleteProgram(particleRenderProgram);
        }
        if (particleUpdateProgram) {
            glDeleteProgram(particleUpdateProgram);
        }
        if (program) {
            glDeleteProgram(program);
        }
        glPopDebugGroup();
        std::cerr << error.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    const Vertex plane[] = {
        {{-5.0f, 0.0f, 5.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{5.0f, 0.0f, 5.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{5.0f, 0.0f, -5.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 5.0f}, {0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{5.0f, 0.0f, 0.0f}, {1.0f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, -5.0f}, {0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-5.0f, 0.0f, 0.0f}, {0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    };
    const GLuint indices[] = {
        0, 5, 4, 8, 5, 1, 6, 4, 4, 6, 2, 7, 8, 4, 7, 3,
    };
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "VAO/VBO/EBO");
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glObjectLabel(GL_BUFFER, vbo, -1, "Plane vertices");
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glObjectLabel(GL_BUFFER, ebo, -1, "Plane indices");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 4, -1, "Particles VAO/VBO/feedback init");
    GLuint particleVAO[2], particleVBO[2], particleFeedback[2];
    glGenVertexArrays(2, particleVAO);
    glGenBuffers(2, particleVBO);
    glGenTransformFeedbacks(2, particleFeedback);
    {
        std::vector<Particle> initialParticles(PARTICLE_COUNT);
        for (GLsizei i = 0; i < PARTICLE_COUNT; ++i) {
            initialParticles[i] = {
                {0.0f, 0.15f, 0.0f}, glm::vec3(0.0f), -0.001f - 3.0f * float(i) / PARTICLE_COUNT, 3.0f};
        }
        for (int i = 0; i < 2; ++i) {
            glBindVertexArray(particleVAO[i]);
            glBindBuffer(GL_ARRAY_BUFFER, particleVBO[i]);
            glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(Particle), i == 0 ? initialParticles.data() : nullptr,
                         GL_DYNAMIC_COPY);
            glObjectLabel(GL_BUFFER, particleVBO[i], -1, i == 0 ? "Particles buffer A" : "Particles buffer B");
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
                                  reinterpret_cast<void*>(offsetof(Particle, position)));
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
                                  reinterpret_cast<void*>(offsetof(Particle, velocity)));
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle),
                                  reinterpret_cast<void*>(offsetof(Particle, age)));
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle),
                                  reinterpret_cast<void*>(offsetof(Particle, lifetime)));
            for (GLuint attribute = 0; attribute < 4; ++attribute) {
                glEnableVertexAttribArray(attribute);
            }
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, particleFeedback[i]);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particleVBO[i]);
            glObjectLabel(GL_TRANSFORM_FEEDBACK, particleFeedback[i], -1,
                          i == 0 ? "Particles feedback A" : "Particles feedback B");
        }
    }
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    glBindVertexArray(0);
    int particleSource = 0;
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "msFramebuffer init");
    const GLuint samples = 4;
    int msWidth, msHeight;
    glfwGetFramebufferSize(window, &msWidth, &msHeight);
    msWidth = std::max(msWidth, 1);
    msHeight = std::max(msHeight, 1);
    GLuint msFBO;
    glGenFramebuffers(1, &msFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
    glObjectLabel(GL_FRAMEBUFFER, msFBO, -1, "MSAA FBO");

    GLuint msColor;
    glGenTextures(1, &msColor);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msColor);
    glObjectLabel(GL_TEXTURE, msColor, -1, "MSAA COLOR");
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, msWidth, msHeight, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msColor, 0);

    GLuint msDepth;
    glGenRenderbuffers(1, &msDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, msDepth);
    glObjectLabel(GL_RENDERBUFFER, msDepth, -1, "MSAA depth renderbuffer");

    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, msWidth, msHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("MSAA FBO incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "Patch height textures");
    const int textureWidth = 256;
    const int textureHeight = 256;
    const int textureLayers = 4;
    const std::size_t layerSize = std::size_t(textureWidth) * textureHeight;
    std::vector<GLubyte> data(layerSize * textureLayers);
    for (int layer = 0; layer < textureLayers; ++layer) {
        for (int y = 0; y < textureHeight; ++y) {
            for (int x = 0; x < textureWidth; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / textureWidth;
                const float v = (static_cast<float>(y) + 0.5f) / textureHeight;
                const float dx = 2.0f * u - 1.0f;
                const float dy = 2.0f * v - 1.0f;
                float h = 0.0f;
                switch (layer) {
                case 0:
                    h = std::exp(-6.0f * (dx * dx + dy * dy));
                    break;
                case 1: {
                    const float left = dx + 0.35f;
                    const float right = dx - 0.35f;
                    h = 0.9f * std::exp(-18.0f * (left * left + 1.5f * dy * dy)) +
                        0.7f * std::exp(-22.0f * (right * right + 1.5f * dy * dy));
                    break;
                }
                case 2: {
                    const float ridge = dy - 0.4f * dx;
                    h = 0.85f * std::exp(-18.0f * ridge * ridge - 2.0f * dx * dx);
                    break;
                }
                case 3:
                    h = std::exp(-5.0f * (dx * dx + dy * dy)) *
                        (0.75f + 0.25f * std::sin(18.0f * dx) * std::sin(16.0f * dy));
                    break;
                }
                const std::size_t index = std::size_t(layer) * layerSize + std::size_t(y) * textureWidth + x;
                data[index] = static_cast<GLubyte>(std::lround(std::clamp(h, 0.0f, 1.0f) * 255.0f));
            }
        }
    }

    GLuint heightTexture;
    glGenTextures(1, &heightTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, heightTexture);
    glObjectLabel(GL_TEXTURE, heightTexture, -1, "Patch height layers");
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, textureWidth, textureHeight, textureLayers, 0, GL_RED, GL_UNSIGNED_BYTE,
                 data.data());
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 3, -1, "Patch material textures");
    const glm::vec3 materialColors[] = {
        {0.22f, 0.42f, 0.12f},
        {0.75f, 0.60f, 0.32f},
        {0.42f, 0.45f, 0.48f},
        {0.85f, 0.91f, 0.95f},
    };
    const float grainStrength[] = {0.30f, 0.15f, 0.22f, 0.06f};
    std::vector<GLubyte> materialData(layerSize * textureLayers * 3);
    for (int layer = 0; layer < textureLayers; ++layer) {
        for (int y = 0; y < textureHeight; ++y) {
            for (int x = 0; x < textureWidth; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / textureWidth;
                const float v = (static_cast<float>(y) + 0.5f) / textureHeight;
                std::uint32_t hash = std::uint32_t(x) + std::uint32_t(y) * textureWidth +
                                     std::uint32_t(layer) * std::uint32_t(layerSize);
                hash = (hash ^ (hash >> 16)) * 0x7feb352du;
                hash = (hash ^ (hash >> 15)) * 0x846ca68bu;
                hash ^= hash >> 16;
                const float noise = static_cast<float>(hash & 0xffffu) / 65535.0f;

                float pattern = 0.0f;
                switch (layer) {
                case 0:
                    pattern = 0.08f * std::sin(90.0f * u + 8.0f * std::sin(40.0f * v));
                    break;
                case 1:
                    pattern = 0.10f * std::sin(45.0f * v + 4.0f * std::sin(10.0f * u));
                    break;
                case 2:
                    pattern = 0.07f * std::sin(30.0f * (u + v) + 8.0f * std::sin(8.0f * v));
                    break;
                case 3:
                    pattern = 0.03f * std::sin(12.0f * u) * std::cos(16.0f * v);
                    break;
                }
                const float brightness = 1.0f + grainStrength[layer] * (noise - 0.5f) + pattern;
                const glm::vec3 color =
                    glm::clamp(materialColors[layer] * brightness, glm::vec3(0.0f), glm::vec3(1.0f));
                const std::size_t index = (std::size_t(layer) * layerSize + std::size_t(y) * textureWidth + x) * 3;
                for (int channel = 0; channel < 3; ++channel) {
                    materialData[index + channel] = static_cast<GLubyte>(std::lround(color[channel] * 255.0f));
                }
            }
        }
    }

    GLuint materialTexture;
    glGenTextures(1, &materialTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, materialTexture);
    glObjectLabel(GL_TEXTURE, materialTexture, -1, "Patch material layers");
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, textureWidth, textureHeight, textureLayers, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, materialData.data());
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPopDebugGroup();

    const GLint viewLoc = glGetUniformLocation(program, "uView");
    const GLint heightLayersLoc = glGetUniformLocation(program, "uHeightLayers");
    const GLint materialLayersLoc = glGetUniformLocation(program, "uMaterialLayers");
    const GLint projectionLoc = glGetUniformLocation(program, "uProjection");
    const GLint CameraPositionLoc = glGetUniformLocation(program, "uCameraPosition");
    const GLint particleDeltaLoc = glGetUniformLocation(particleUpdateProgram, "uDeltaTime");
    const GLint particleTimeLoc = glGetUniformLocation(particleUpdateProgram, "uTime");
    const GLint particleViewLoc = glGetUniformLocation(particleRenderProgram, "uView");
    const GLint particleProjectionLoc = glGetUniformLocation(particleRenderProgram, "uProjection");
    const GLint particleViewportLoc = glGetUniformLocation(particleRenderProgram, "uViewportHeight");

    glPopDebugGroup();
    Camera camera;
    double lastTime = glfwGetTime();
    int state = GL_FILL;
    bool wasPressed = false, isPressed = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(std::min(now - lastTime, 0.1));
        lastTime = now;
        camera.Update(window, dt);
        isPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !wasPressed) {
            state = GL_POINT + ((state + 1) % GL_POINT) % 3;
            glPolygonMode(GL_FRONT_AND_BACK, state);
        }
        wasPressed = isPressed;
        if (glfwWindowShouldClose(window)) {
            break;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        if (width != msWidth || height != msHeight) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "MSAA resize");
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msColor);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, GL_TRUE);
            glBindRenderbuffer(GL_RENDERBUFFER, msDepth);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
            msWidth = width;
            msHeight = height;
            glPopDebugGroup();
        }

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 4, -1, "Particles update / transform feedback");
        const int particleDestination = 1 - particleSource;
        glUseProgram(particleUpdateProgram);
        glUniform1f(particleDeltaLoc, dt);
        glUniform1f(particleTimeLoc, static_cast<float>(now));
        glBindVertexArray(particleVAO[particleSource]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, particleFeedback[particleDestination]);
        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        particleSource = particleDestination;
        glPopDebugGroup();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "1st color pass");
        glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.Forward(), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 projection =
            glm::perspective(glm::radians(60.0f), static_cast<float>(width) / height, 0.1f, 200.0f);
        glUseProgram(program);
        glUniform1i(heightLayersLoc, 0);
        glUniform1i(materialLayersLoc, 1);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(CameraPositionLoc, 1, glm::value_ptr(camera.pos));
        glBindVertexArray(vao);
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        glDrawElements(GL_PATCHES, 16, GL_UNSIGNED_INT, nullptr);
        glPopDebugGroup();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 5, -1, "Particles render");
        glUseProgram(particleRenderProgram);
        glUniformMatrix4fv(particleViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(particleProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(particleViewportLoc, static_cast<float>(height));
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glBindVertexArray(particleVAO[particleSource]);
        glDrawTransformFeedback(GL_POINTS, particleFeedback[particleSource]);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
        glPopDebugGroup();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 6, -1, "MSAA resolve");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glPopDebugGroup();
        glfwSwapBuffers(window);
    }
    glDeleteTransformFeedbacks(2, particleFeedback);
    glDeleteBuffers(2, particleVBO);
    glDeleteVertexArrays(2, particleVAO);
    glDeleteProgram(particleUpdateProgram);
    glDeleteProgram(particleRenderProgram);
    glDeleteFramebuffers(1, &msFBO);
    glDeleteTextures(1, &msColor);
    glDeleteRenderbuffers(1, &msDepth);
    glDeleteTextures(1, &heightTexture);
    glDeleteTextures(1, &materialTexture);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
