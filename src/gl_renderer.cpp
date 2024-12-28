#include "gl_renderer.h"

// To Load PNG Files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <gl/gl.h>
#include "input.h"

#include <iostream>
#include <vector>

// ################################################################
//                       OpenGL Constants
// ################################################################
const char *TEXTURE_PATH = "assets/textures/TEXTURE_ATLAS.png";

// ################################################################
//                       OpenGL Structs
// ################################################################
struct GLContext
{
    GLuint programID;
    GLuint textureID;
};

// ################################################################
//                       OpenGL Globals
// ################################################################
static GLContext glContext;

// ################################################################
//                       OpenGL Functions
// ################################################################
GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

static void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar *message, const void *user)
{
    if (severity == GL_DEBUG_SEVERITY_LOW ||
        severity == GL_DEBUG_SEVERITY_MEDIUM ||
        severity == GL_DEBUG_SEVERITY_HIGH)
    {
        SM_ASSERT(false, "OpenGL Error: %s", message);
    }
    else
    {
        SM_TRACE((char *)message);
    }
}

bool gl_init(BumpAllocator *transientStorage)
{
    load_gl_functions();
    glCheckError();

    std::string version = (char*)glGetString(GL_VERSION);
    std::cout << "GL_VERSION: " << version << std::endl;

    glDebugMessageCallback(&gl_debug_callback, nullptr);
    glCheckError();
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glCheckError();
    glEnable(GL_DEBUG_OUTPUT);
    glCheckError();

    GLuint vertShaderID = glCreateShader(GL_VERTEX_SHADER);
    glCheckError();
    GLuint fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glCheckError();

    int fileSize = 0;
    char *vertShader = read_file("assets/shaders/quad.vert", &fileSize, transientStorage);
    char *fragShader = read_file("assets/shaders/quad.frag", &fileSize, transientStorage);

    if (!vertShader || !fragShader)
    {
        SM_ASSERT(false, "Failed to load shaders");
        return false;
    }

    glShaderSource(vertShaderID, 1, &vertShader, 0);
    glCheckError();
    glShaderSource(fragShaderID, 1, &fragShader, 0);
    glCheckError();

    glCompileShader(vertShaderID);
    glCheckError();
    glCompileShader(fragShaderID);
    glCheckError();

    // Test if Vertex shader compiled successfully
    {
        // int success;
        GLint isCompiled = 0;

        glGetShaderiv(vertShaderID, GL_COMPILE_STATUS, &isCompiled);
        glCheckError();
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(vertShaderID, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> errorLog(maxLength);
            glGetShaderInfoLog(vertShaderID, maxLength, &maxLength, &errorLog[0]);
            SM_ASSERT(false, "Failed to compile vertex shader: %s", &errorLog[0]);
            
        }
    }
    // Test if Fragment shader compiled successfully
    {
        int success;
        char shaderLog[2048] = {};

        glGetShaderiv(fragShaderID, GL_COMPILE_STATUS, &success);
        glCheckError();
        if (!success)
        {
            glGetShaderInfoLog(fragShaderID, 2048, 0, shaderLog);
            SM_ASSERT(false, "Failed to compile fragment shader: %s", shaderLog);
        }
    }

    glContext.programID = glCreateProgram();
    glCheckError();
    glAttachShader(glContext.programID, vertShaderID);
    glCheckError();
    glAttachShader(glContext.programID, fragShaderID);
    glCheckError();
    glLinkProgram(glContext.programID);
    glCheckError();
    GLint isLinked = 0;
    glGetProgramiv(glContext.programID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(glContext.programID, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(glContext.programID, maxLength, &maxLength, &infoLog[0]);

        // The program is useless now. So delete it.
        glDeleteProgram(glContext.programID);

        // Provide the infolog in whatever manner you deem best.
        // Exit with failure.
    }

    glDetachShader(glContext.programID, vertShaderID);
    glCheckError();
    glDetachShader(glContext.programID, fragShaderID);
    glCheckError();
    glDeleteShader(vertShaderID);
    glCheckError();
    glDeleteShader(fragShaderID);
    glCheckError();

    // This has to be doen, otherwise OpenGL will not draw anything
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glCheckError();
    glBindVertexArray(VAO);
    glCheckError();

    // Texture Loading using STBI
    {
        int width, height, channels;
        char *data = (char *)stbi_load(TEXTURE_PATH, &width, &height, &channels, 4);

        if (!data)
        {
            SM_ASSERT(false, "Failed to load texture");
            return false;
        }

        glGenTextures(1, &glContext.textureID);
        glCheckError();
        std::cout << glGetError() << std::endl;
        glActiveTexture(GL_TEXTURE0);
        glCheckError();
        glBindTexture(GL_TEXTURE_2D, glContext.textureID);
        glCheckError();

        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glCheckError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glCheckError();
        // This setting only matters when using the GLSL textsure() function
        // When you use the texelFetch() this setting has no effect,
        // because texelFetch is designed for this purpose
        // See: https://interactiveimmersive.io/blog/glsl/glsl-data-tricks/
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glCheckError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glCheckError();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        std::cout << glGetError() << std::endl;
        glCheckError();

        stbi_image_free(data);
        glCheckError();
    }

    // sRGB output (even if input texture is non-sRGB -> don't rely on texture used)
    // Your font is not using sRGB, for example (not that it matters, there, because no actual color is sampled from it)
    // But this could prevent some future bug when you start mixing different types of textures
    // Of course, you still need to correctly set the image file sourc eformat when using glTexImage2d()
    glEnable(GL_FRAMEBUFFER_SRGB);
    glCheckError();
    glDisable(0x809D); // disable multisampling
    glCheckError();

    // Depth Testing
    glEnable(GL_DEPTH_TEST);
    glCheckError();
    glEnable(GL_GREATER);

    // Use Program renderdoc
    glUseProgram(glContext.programID);

    return true;
}

void gl_render()
{
    glClearColor(119.0f / 255.0f, 33.0f / 255.0f, 111.0f / 255.0f, 1.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, input.screenSizeX, input.screenSizeY);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}