#include "platform.h"
#include "input.h"
#include "hweg_lib.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "wglext.h"
#include "glcorearb.h"

// ################################################################
//                       Windows Globals
// ################################################################
static HWND window;
static HDC dc;
// ################################################################
//                       Platform Implementations
// ################################################################
LRESULT CALLBACK windows_window_callback(HWND window, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_CLOSE:
    {
        running = false;
        break;
    }

    case WM_SIZE:
    {
        RECT rect = {};
        GetClientRect(window, &rect);
        input.screenSizeX = rect.right - rect.left;
        input.screenSizeY = rect.bottom - rect.top;

        break;
    }

    default:
    {
        // Let windows handle the default input for now
        result = DefWindowProcA(window, msg, wParam, lParam);
    }
    }

    return result;
}

bool platform_create_window(int width, int height, char *title)
{
    HINSTANCE instance = GetModuleHandleA(0);

    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(instance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // This means we decide the look of the cursor(arrow)
    wc.lpszClassName = title;                 // This is NOT the title, just a unique identifier(ID)
    wc.lpfnWndProc = windows_window_callback; // Callback for Input into the Window

    if (!RegisterClassA(&wc))
    {
        return false;
    }

    // WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
    int dwStyle = WS_OVERLAPPEDWINDOW;

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

    // Fake Window initializing OpenGL
    {
        window = CreateWindowExA(0, title, // This references lpszClassName from wc
                                 title,    // This is the actual Title
                                 dwStyle,
                                 100,
                                 100,
                                 width,
                                 height,
                                 NULL, // parent
                                 NULL, // menu
                                 instance,
                                 NULL); // lpParam

        if (window == NULL)
        {
            SM_ASSERT(false, "Failed to create Windows Window");
            return false;
        }

        HDC fakeDC = GetDC(window);
        if (!fakeDC)
        {
            SM_ASSERT(false, "Failed to get HDC");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd =
            {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
                32,                                                         // Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24, // Number of bits for the depthbuffer
                8,  // Number of bits for the stencilbuffer
                0,  // Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0};

        // PIXELFORMATDESCRIPTOR pfd = {0};
        // pfd.nSize = sizeof(pfd);
        // pfd.nVersion = 1;
        // pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        // pfd.iPixelType = PFD_TYPE_RGBA;
        // pfd.cColorBits = 32;
        // pfd.cAlphaBits = 8;
        // pfd.cDepthBits = 24;

        int pixelFormat = ChoosePixelFormat(fakeDC, &pfd);
        if (!pixelFormat)
        {
            SM_ASSERT(false, "Failed to choose pixel Format");
            return false;
        }

        if (!SetPixelFormat(fakeDC, pixelFormat, &pfd))
        {
            SM_ASSERT(false, "Failed to set pixel format");
            return false;
        }

        // Create a Handle to a fake OpenGL Rendering Context
        HGLRC fakeRC = wglCreateContext(fakeDC);
        if (!fakeRC)
        {
            SM_ASSERT(false, "Failed to create Render context");
            return false;
        }

        if (!wglMakeCurrent(fakeDC, fakeRC))
        {
            SM_ASSERT(false, "Failed to make current");
            return false;
        }

        wglChoosePixelFormatARB =
            (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_function("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_function("wglCreateContextAttribsARB");

        if (!wglCreateContextAttribsARB || !wglChoosePixelFormatARB)
        {
            SM_ASSERT(false, "Failed to load OpenGL functions");
            return false;
        }

        // Clean up the take stuff
        wglMakeCurrent(fakeDC, 0);
        wglDeleteContext(fakeRC);
        ReleaseDC(window, fakeDC);

        // Can't reuse the same (Device)Context,
        // because we already called "SetPixelFormat"
        DestroyWindow(window);
    }

    // Actual OpenGL initialization
    {
        // Add in the border size of the window
        {
            RECT borderRect = {};
            AdjustWindowRectEx(&borderRect, dwStyle, 0, 0);

            width += borderRect.right - borderRect.left;
            height += borderRect.bottom - borderRect.top;
        }

        window = CreateWindowExA(0, title, // This references lpszClassName from wc
                                 title,    // This is the actual Title
                                 dwStyle,
                                 100,
                                 100,
                                 width,
                                 height,
                                 NULL, // parent
                                 NULL, // menu
                                 instance,
                                 NULL); // lpParam

        if (window == NULL)
        {
            SM_ASSERT(false, "Failed to create Windows Window");
            return false;
        }

        dc = GetDC(window);
        if (!dc)
        {
            SM_ASSERT(false, "Failed to get DC");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd =
            {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
                32,                                                         // Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24, // Number of bits for the depthbuffer
                8,  // Number of bits for the stencilbuffer
                0,  // Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0};

        const int attribList[] =
            {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_ARB_create_context_profile, GL_TRUE,
                WGL_ARB_pixel_format_float, GL_TRUE,
                WGL_ARB_framebuffer_sRGB, GL_TRUE,
                WGL_ARB_multisample, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_DEPTH_BITS_ARB, 24,
                WGL_STENCIL_BITS_ARB, 8,
                0, // Terminate with 0, otherwise OpenGL will throw an Error!
            };

        int pixelFormat;
        UINT numFormats;
        BOOL result = TRUE;
        result = wglChoosePixelFormatARB(dc, attribList, NULL, 1, &pixelFormat, &numFormats);
        printf("result: %d\n", (BOOL)result);
        if (result == FALSE)
        {
            SM_ASSERT(0, "Failed to Choose Pixel Format");
            return false;
        }

        // int pixelFormat = ChoosePixelFormat(dc, &pfd);
        // if (!pixelFormat)
        // {
        //     // char errorStr[1024];
        //     // FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, errorStr, 255, NULL);
        //     // printf("Error: %s\n", errorStr);
        //     SM_ASSERT(false, "Failed to choose pixel Format");
        //     return false;
        // }
        // printf("Pixel Format: %d\n", pixelFormat);

        DescribePixelFormat(dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        if (!SetPixelFormat(dc, pixelFormat, &pfd))
        {
            // char errorStr[1024];
            // FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, errorStr, 255, NULL);
            // printf("Error: %s\n", errorStr);
            SM_ASSERT(0, "Failed to SetPixelFormat");
            return true;
        }

        const int contextAttribs[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
                0 // Terminate the Array
            };

        //HGLRC rc = wglCreateContext(dc);
        HGLRC rc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
        if (!rc)
        {
            SM_ASSERT(0, "Failed to create Render Context for OpenGL");
            return false;
        }

        if (!wglMakeCurrent(dc, rc))
        {
            SM_ASSERT(0, "Failed to wglMakeCurrent");
            return false;
        }
    }

    ShowWindow(window, SW_SHOW);

    return true;
}

void platform_update_window()
{
    MSG msg;

    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg); // Calls the callback specified when creating the window
    }
}

void *platform_load_gl_function(char *funName)
{
    PROC proc = wglGetProcAddress("glCreateProgram");
    if (!proc)
    {
        static HMODULE openglDLL = LoadLibraryA("opengl32.dll");
        proc = GetProcAddress(openglDLL, funName);

        if (!proc)
        {
            SM_ASSERT(false, "Failed to load gl function %s", "glCreateProgram");
            return nullptr;
        }
    }

    return (void *)proc;
}

void platform_swap_buffers()
{
    SwapBuffers(dc);
}