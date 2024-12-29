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
static HGLRC GlobalOpenGLRC;
static HDC GlobalDC;
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

// Declarations for WGL Extensions
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

int Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(casey): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if HANDMADE_INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

void Win32CreateOpenGLContextForWorkerThread(void)
{
    if(wglCreateContextAttribsARB)
    {
        HDC WindowDC = GlobalDC;
        HGLRC ShareContext = GlobalOpenGLRC;
        HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs);
        if(ModernGLRC)
        {
            if(wglMakeCurrent(WindowDC, ModernGLRC))
            {
                // TODO(casey): Fatal error?
            }
            else
            {
                SM_ASSERT(false, "Unable to create texture download context");
                return;
            }
        }
    }
}

void Win32SetPixelFormat(HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if (wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
            {
                WGL_DRAW_TO_WINDOW_ARB,
                GL_TRUE,
                WGL_ACCELERATION_ARB,
                WGL_FULL_ACCELERATION_ARB,
                WGL_SUPPORT_OPENGL_ARB,
                GL_TRUE,
#if CELESTECLONE_STREAMING
                WGL_DOUBLE_BUFFER_ARB,
                GL_FALSE,
#else
                WGL_DOUBLE_BUFFER_ARB,
                GL_TRUE,
#endif
                WGL_PIXEL_TYPE_ARB,
                WGL_TYPE_RGBA_ARB,
                WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,
                GL_TRUE,
                0,
            };

        float FloatAttribList[] = {0};

        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1,
                                &SuggestedPixelFormatIndex, &ExtendedPick);
    }

    if (!ExtendedPick)
    {
        // TODO(casey): Hey Raymond Chen - what's the deal here?
        // Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
#if CELESTECLONE_STREAMING
        // NOTE(casey): PFD_DOUBLEBUFFER appears to prevent OBS from reliably streaming the window
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
#else
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
#endif
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

void Win32LoadWGLExtensions(void)
{
    WNDCLASSA WindowClass = {};

    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "Dummy_WGL_dont_use";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Dummy_WGL_dont_use",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            WindowClass.hInstance,
            0);
        
        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(WindowDC);
        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        if(wglMakeCurrent(WindowDC, OpenGLRC))        
        {
            wglChoosePixelFormatARB =
                (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_function("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
                (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_function("wglCreateContextAttribsARB");

            wglMakeCurrent(0, 0);
        }

        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }

}

HGLRC Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();

    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        Win32SetPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }
    
    if(!OpenGLRC)
    {
        OpenGLRC = wglCreateContext(WindowDC);
    }

    if(!wglMakeCurrent(WindowDC, OpenGLRC))
    {
        SM_ASSERT(false, "Failed to make current");
        return false;
    }
    
    return(OpenGLRC);
}

//bool platform_create_window(int width, int height, char *title)
bool platform_create_window(int width, int height, char* title)
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

    int dwStyle = WS_OVERLAPPEDWINDOW;

    RECT borderRect = {};
    AdjustWindowRectEx(&borderRect, dwStyle, 0, 0);

    width += borderRect.right - borderRect.left;
    height += borderRect.bottom - borderRect.top;

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

    GlobalDC = GetDC(window);
    if (!GlobalDC)
    {
        SM_ASSERT(false, "Failed to get DC");
        return false;
    }
    // Win32CreateOpenGLContextForWorkerThread();
    GlobalOpenGLRC = Win32InitOpenGL(GlobalDC);
    if (!GlobalOpenGLRC)
    {
        SM_ASSERT(false, "Failed to get GlobalOpenGLRC");
        return false;
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
    SwapBuffers(GlobalDC);
}