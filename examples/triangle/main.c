//#include "vulkan/vulkan.h"
#include "cugl/cugl.h"
#include "GLFW/glfw3.h"

#include "stdio.h"
#include "stdlib.h"

void MessageCallbackGL(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    printf("%s\n", message);
}

const char SourceV[] =
"#version 460\n"

"layout(location = 0) in vec2 inPosition;\n"
"layout(location = 1) in vec3 inColor;\n"

"layout(location = 0) out vec3 fragColor;\n"

"void main() {\n"
"   gl_Position = vec4(inPosition, 0.0, 1.0);\n"
"    fragColor = inColor;\n"
"}\n";

const char SourceF[] =
"#version 460\n"

"layout(location = 0) in vec3 fragColor;\n"
"layout(location = 0) out vec4 outColor;\n"

"void main() {\n"
"    outColor = vec4(fragColor, 1.0);\n"
"}\n";

int CreateShaderProgram(GLuint *OutProgram) {
    GLuint S[2];
    const char *Sources[] = { SourceV, SourceF };
    GLint Lengths[] = { sizeof(SourceV), sizeof(SourceF) };
    GLenum Types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    for(int I = 0; I < 2; ++I) {
        S[I] = glCreateShader(Types[I]);
        glShaderSource(S[I], 1, &Sources[I], Lengths + I);
        
        glCompileShader(S[I]);

        GLint Status = 0;
        glGetShaderiv(S[I], GL_COMPILE_STATUS, &Status);
        if(Status == GL_FALSE) {
            GLint InfoLength = 0;
            glGetShaderiv(S[I], GL_INFO_LOG_LENGTH, &InfoLength);
            printf("Compile length: %d\n", InfoLength);
            char Buf[4096] = {0};
            GLsizei L;
            glGetShaderInfoLog(S[I], 4096, &L, Buf);
            printf("%s", Buf);
            return 1;
        }
    }

    GLuint P = glCreateProgram();
    glAttachShader(P, S[0]);
    glAttachShader(P, S[1]);
    glLinkProgram(P);
    
    GLint Status = 0;
    glGetProgramiv(P, GL_LINK_STATUS, &Status);
    if(Status == GL_FALSE) {
        GLint InfoLength = 0;
        glGetProgramiv(P, GL_INFO_LOG_LENGTH, &InfoLength);
        printf("Link length: %d\n", InfoLength);
        char Buf[4096] = {0};
        GLsizei L;
        glGetProgramInfoLog(P, 4096, &L, Buf);
        printf("%s", Buf);
        return 1;
    }

    *OutProgram = P;

    //printf("%d\n", glGetUniformLocation(P, "rectColor"));
    return 0;
}

#define AssertMessageGoto(Condition, Label, Format, ...) if(!(Condition)) { printf(Format, ##__VA_ARGS__); goto Label; }

VkResult SurfaceCreateGLFW(VkInstance Instance, const VkAllocationCallbacks *AllocationCallbacks, VkSurfaceKHR *OutSurface, void *User) {
    return glfwCreateWindowSurface(Instance, User, AllocationCallbacks, OutSurface);
}

int main() {
    glfwInitVulkanLoader(vkGetInstanceProcAddr);

    AssertMessageGoto(glfwInit(), label_Exit, "glfwInit failed\n");

#ifdef VULKAN_EXPLICIT_LAYERS_PATH
        AssertMessageGoto(setenv("VK_ADD_LAYER_PATH", VULKAN_EXPLICIT_LAYERS_PATH, 1) == 0, label_Exit, "Failed to set VK_ADD_LAYER_PATH.\n");
        //printf("%s\n", VULKAN_EXPLICIT_LAYERS_PATH);
#endif
#ifdef VULKAN_DRIVER_FILES
        AssertMessageGoto(setenv("VK_DRIVER_FILES", VULKAN_DRIVER_FILES, 1) == 0, label_Exit, "Failed to set VULKAN_DRIVER_FILES.\n");
        //printf("%s\n", VULKAN_DRIVER_FILES);
#endif

    if(glfwVulkanSupported() == 0) {
        printf("Vulkan not supported\n");
        return 1;
    }

    uint32_t RequiredInstanceExtensionCount = 0;
    const char **RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&RequiredInstanceExtensionCount);
    if(RequiredInstanceExtensions == 0) {
        printf("glfwGetRequiredInstanceExtensions null\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *Window = glfwCreateWindow(1600, 900, "vulkangl", 0, 0);

    glDebugMessageCallback(MessageCallbackGL, 0);

    context_create_params Params = {
        .RequiredInstanceExtensions = RequiredInstanceExtensions,
        .RequiredInstanceExtensionCount = RequiredInstanceExtensionCount,
        .CreateSurface = SurfaceCreateGLFW,
        .User = Window,
    };

    if(cuglCreateContext(&Params)) {
        printf("Context creation failed\n");
        return 1;
    }

    float Vertices[] = {
        0.0f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f, 1.0f
    };

    GLuint Vao = 0;
    glGenVertexArrays(1, &Vao);
    glBindVertexArray(Vao);
    GLuint Vbo = 0, Ibo = 0;
    glGenBuffers(1, &Vbo);
    //glGenBuffers(1, &Ibo);
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (const void *)(2*sizeof(float)));

    GLuint Program;
    if(CreateShaderProgram(&Program)) {
        printf("Shader program creation failed\n");
        return 1;
    }

    glClearColor(1.0f, 0.5f, 0.0f, 1.0f);

    glUseProgram(Program);
    glBindVertexArray(Vao);

    while(glfwWindowShouldClose(Window) == 0) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        cuglSwapBuffers();
    }

label_Exit:
    glfwTerminate();

    return 0;
}