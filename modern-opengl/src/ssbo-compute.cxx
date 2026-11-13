#include "common.hxx"

#include <glm/vec4.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr GLuint ParticleCount = 2050;
constexpr GLuint WorkgroupSize = 64;

struct alignas(16) Particle {
    glm::vec4 position; // xyz - pos, w - age
    glm::vec4 velocity; // xyz - vel, w - lifetime
};
static_assert(sizeof(Particle) == 32);
static_assert(offsetof(Particle, velocity) == 16);

struct ParticleSystem {
    GLuint buffer = 0;
    GLuint vao = 0;
    GLuint updateProgram = 0;
    GLuint renderProgram = 0;
    GLint dtLocation = -1;
    GLint aspectLocation = -1;

    ParticleSystem() = default;
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    ~ParticleSystem() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &buffer);
        if (renderProgram) {
            glDeleteProgram(renderProgram);
        }
        if (updateProgram) {
            glDeleteProgram(updateProgram);
        }
    }
};

GLuint CreateParticleBuffer() {
    std::vector<Particle> particles(ParticleCount);
    for (GLuint i = 0; i < ParticleCount; ++i) {
        particles[i] = {{0.0f, -0.7f, 0.0f, -2.0f * static_cast<float>(i) / ParticleCount}, {0.0f, 1.5f, 0.0f, 2.0f}};
    }

    GLuint buffer = 0;
    glCreateBuffers(1, &buffer);
    glNamedBufferStorage(buffer, particles.size() * sizeof(Particle), particles.data(), 0);
    glObjectLabel(GL_BUFFER, buffer, -1, "Particles: SSBO and vertex source");
    return buffer;
}

GLuint CreateParticleArray(GLuint buffer) {
    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, buffer, 0, sizeof(Particle));
    glVertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, offsetof(Particle, position));
    glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, GL_FALSE, offsetof(Particle, velocity));
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    return vao;
}

void InitializeParticles(ParticleSystem& particles) {
    particles.updateProgram = CreateComputeProgram("particles.comp");
    particles.renderProgram = CreateProgram("particles.vert", "particles.frag");
    particles.dtLocation = glGetUniformLocation(particles.updateProgram, "dt");
    particles.aspectLocation = glGetUniformLocation(particles.renderProgram, "aspect");
    const GLint countLocation = glGetUniformLocation(particles.updateProgram, "particleCount");
    glProgramUniform1ui(particles.updateProgram, countLocation, ParticleCount);

    particles.buffer = CreateParticleBuffer();
    particles.vao = CreateParticleArray(particles.buffer);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glClearColor(0.025f, 0.035f, 0.055f, 1.0f);
}

void UpdateParticles(const ParticleSystem& particles, float dt) {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particles.buffer);
    glProgramUniform1f(particles.updateProgram, particles.dtLocation, dt);
    glUseProgram(particles.updateProgram);
    glDispatchCompute((ParticleCount + WorkgroupSize - 1) / WorkgroupSize, 1, 1);
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void RenderParticles(const ParticleSystem& particles, float aspect) {
    glClear(GL_COLOR_BUFFER_BIT);
    glProgramUniform1f(particles.renderProgram, particles.aspectLocation, aspect);
    glUseProgram(particles.renderProgram);
    glBindVertexArray(particles.vao);
    glDrawArrays(GL_POINTS, 0, ParticleCount);
}

void Run(GLFWwindow* window, const Options& options) {
    ParticleSystem particles;
    InitializeParticles(particles);

    double previousTime = glfwGetTime();
    int frame = 0;
    while (!glfwWindowShouldClose(window) && (!options.frames || frame < options.frames)) {
        int width = 0;
        int height = 0;
        if (!BeginFrame(window, width, height)) {
            continue;
        }
        const double now = glfwGetTime();
        const float dt = std::min(static_cast<float>(now - previousTime), 0.05f);
        previousTime = now;

        UpdateParticles(particles, dt);
        RenderParticles(particles, static_cast<float>(width) / height);
        glfwSwapBuffers(window);
        ++frame;
    }
}

}

int main(int argc, char** argv) {
    GLFWwindow* window = nullptr;
    try {
        const Options options = ReadOptions(argc, argv);
        window = CreateWindow("SSBO + compute + DSA");
        Run(window, options);
        DestroyWindow(window);
        return HasDebugErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        DestroyWindow(window);
        return EXIT_FAILURE;
    }
}
