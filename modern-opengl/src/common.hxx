#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct Options {
    int frames = 0;
};

Options ReadOptions(int argc, char** argv);
GLuint CreateProgram(const char* vertex, const char* fragment);
GLuint CreateComputeProgram(const char* compute);
GLFWwindow* CreateWindow(const char* title);
void DestroyWindow(GLFWwindow* window);
bool BeginFrame(GLFWwindow* window, int& width, int& height);
bool HasDebugErrors();
