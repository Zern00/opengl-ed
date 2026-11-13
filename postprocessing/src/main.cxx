#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <format>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <vector>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

struct Camera {
    glm::vec3 pos;
    glm::vec3 view;
    glm::vec3 up;
};

struct Vertex {
    glm::vec3 position;
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
    GLint success = GL_FALSE;
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

GLuint CreateProgram(const char* vertex, const char* fragment) {
    const auto directory = std::filesystem::path(SHADER_DIR);
    const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, directory / vertex);
    GLuint fragmentShader = 0;
    try {
        fragmentShader = CompileShader(GL_FRAGMENT_SHADER, directory / fragment);
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }
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

GLuint CreateCheckerboard() {
    const GLuint textureWidth = 800;
    const GLuint textureHeight = 800;
    std::vector<GLubyte> data(textureWidth * textureHeight * 4);

    for (size_t y = 0; y < textureHeight; ++y) {
        for (size_t x = 0; x < textureWidth; ++x) {

            int ceilX = x / 8;
            int ceilY = y / 8;

            const bool isColor = (ceilX + ceilY) % 2;
            const GLubyte color = isColor ? 255 : 0;
            const GLubyte objectId = 1;
            const size_t offset = (y * textureWidth + x) * 4;

            data[offset + 0] = color;
            data[offset + 1] = color;
            data[offset + 2] = color;
            data[offset + 3] = objectId;
        }
    }

    GLuint textureUnit;
    glGenTextures(1, &textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureUnit);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    return textureUnit;
}

int main() {
    if (!glfwInit()) {
        const char* description;
        glfwGetError(&description);
        throw std::runtime_error(std::format("glfwInitError: {}", description));
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "postprocessing", nullptr, nullptr);
    if (!window) {
        const char* description;
        glfwGetError(&description);
        throw std::runtime_error(std::format("WindowInit error: {}", description));
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

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

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    const GLfloat quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,

        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
    };

    GLuint vboFramebufferQuad;
    glGenBuffers(1, &vboFramebufferQuad);
    glBindBuffer(GL_ARRAY_BUFFER, vboFramebufferQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);

    GLuint vaoFramebufferQuad;
    glGenVertexArrays(1, &vaoFramebufferQuad);
    glBindVertexArray(vaoFramebufferQuad);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)(sizeof(GLfloat) * 2));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    const GLuint textureUnit = CreateCheckerboard();

    GLuint samplerUnit;
    glGenSamplers(1, &samplerUnit);
    glActiveTexture(GL_TEXTURE0);
    glSamplerParameteri(samplerUnit, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(samplerUnit, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(samplerUnit, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(samplerUnit, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glBindSampler(0, samplerUnit);

    GLuint postprocessFBO;
    glGenFramebuffers(1, &postprocessFBO);

    GLuint postprocessColorTexture;
    glGenTextures(1, &postprocessColorTexture);
    glBindTexture(GL_TEXTURE_2D, postprocessColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH / 2, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint postprocessDepthRBO;
    glGenRenderbuffers(1, &postprocessDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, postprocessDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH / 2, HEIGHT);

    glBindFramebuffer(GL_FRAMEBUFFER, postprocessFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postprocessColorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, postprocessDepthRBO);

    const GLenum preprocessDrawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, preprocessDrawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("postprocessFBO is incomplete");
    }

    GLuint mainFBO;
    glGenFramebuffers(1, &mainFBO);

    GLuint mainColorTexture;
    glGenTextures(1, &mainColorTexture);
    glBindTexture(GL_TEXTURE_2D, mainColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH / 2, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint mainDepthRBO;
    glGenRenderbuffers(1, &mainDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, mainDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH / 2, HEIGHT);

    GLuint mainObjIndTexture;
    glGenTextures(1, &mainObjIndTexture);
    glBindTexture(GL_TEXTURE_2D, mainObjIndTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, WIDTH / 2, HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainColorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mainObjIndTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mainDepthRBO);

    const GLenum mainDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, mainDrawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("mainFBO is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureUnit);

    Camera camera1({0.0f, -3.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), static_cast<float>(WIDTH) / (2 * HEIGHT), 0.01f, 100.0f);
    glm::mat4 view1 = glm::lookAt(camera1.pos, camera1.pos + camera1.view, camera1.up);
    const GLuint program1_main = CreateProgram("main-shader1.vert", "main-shader1.frag");
    const GLuint program1_postprocess = CreateProgram("postprocess-shader1.vert", "postprocess-shader1.frag");
    const GLuint program_outline = CreateProgram("main-shader1.vert", "outline.frag");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLint projLoc1_main = glGetUniformLocation(program1_main, "proj");
    GLint viewLoc_main = glGetUniformLocation(program1_main, "view");
    GLint modelLoc_main = glGetUniformLocation(program1_main, "model");

    GLint framebufferColorTextureLoc = glGetUniformLocation(program1_postprocess, "framebufferColorTexture");
    GLint framebufferIndTextureLoc = glGetUniformLocation(program1_postprocess, "framebufferIndTexture");
    GLint mainTextureLoc = glGetUniformLocation(program1_main, "mainTexture");

    GLint projLoc_outline = glGetUniformLocation(program_outline, "proj");
    GLint viewLoc_outline = glGetUniformLocation(program_outline, "view");
    GLint modelLoc_outline = glGetUniformLocation(program_outline, "model");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureUnit);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mainColorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mainObjIndTexture);

    while (!glfwWindowShouldClose(window)) {
        const GLfloat angle = static_cast<GLfloat>(glfwGetTime());
        const glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));

        glUseProgram(program1_main);
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
        glViewport(0, 0, WIDTH / 2, HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);

        const GLfloat clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glClearBufferfv(GL_COLOR, 0, clearColor);
        const GLfloat clearInd[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        glClearBufferfv(GL_COLOR, 1, clearInd);
        glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 0);

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureUnit);
        glUniform1i(mainTextureLoc, 0);
        glUniformMatrix4fv(projLoc1_main, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(viewLoc_main, 1, GL_FALSE, glm::value_ptr(view1));
        glUniformMatrix4fv(modelLoc_main, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sizeof(cubeIndices) / sizeof(cubeIndices[0])),
                       GL_UNSIGNED_INT, nullptr);

        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);

        const GLenum outlineDrawBuffer[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, outlineDrawBuffer);

        const glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.01f));
        glUseProgram(program_outline);
        glUniformMatrix4fv(projLoc_outline, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(viewLoc_outline, 1, GL_FALSE, glm::value_ptr(view1));
        glUniformMatrix4fv(modelLoc_outline, 1, GL_FALSE, glm::value_ptr(outlineModel));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sizeof(cubeIndices) / sizeof(cubeIndices[0])),
                       GL_UNSIGNED_INT, nullptr);

        glDrawBuffers(2, mainDrawBuffers);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, postprocessFBO);
        glViewport(0, 0, WIDTH / 2, HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program1_postprocess);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mainColorTexture);
        glUniform1i(framebufferColorTextureLoc, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mainObjIndTexture);
        glUniform1i(framebufferIndTextureLoc, 2);

        glBindVertexArray(vaoFramebufferQuad);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO);
        glBlitFramebuffer(0, 0, WIDTH / 2, HEIGHT, 0, 0, WIDTH / 2, HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, postprocessFBO);
        glBlitFramebuffer(0, 0, WIDTH / 2, HEIGHT, WIDTH / 2, 0, WIDTH, HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
