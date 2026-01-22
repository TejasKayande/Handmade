#define NOMINMAX
#include <windows.h>

#include <stdint.h>
#include <math.h>

#define local static 
#define global static 
#define internal static 


struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void      *Memory;
    int        Width;
    int        Height;
    int        BytesPerPixel;
    int        Pitch;
};

struct win32_window_dimention {
    int Width;
    int Height;
};

global bool Running;
global win32_offscreen_buffer GlobalBackbuffer;

global unsigned int XOFF, YOFF;

internal win32_window_dimention Win32GetWindowDimention(HWND Window) {

    win32_window_dimention Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width  = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void RenderGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {

    // TODO(Tejas): What is better pass by reference or pass by value?

    uint8_t *Row = (uint8_t*) Buffer->Memory;

    for (int Y = 0; Y < Buffer->Height; Y++) {

        uint32_t *Pixel = (uint32_t*) Row;

        for (int X = 0; X < Buffer->Width; X++) {
            // AA RR GG BB 

            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset);

            *Pixel = (Green << 8) | Blue;

            // float fx = (float)X / Buffer->Width;
            // float fy = (float)Y / Buffer->Height;

            // float cx = fx - 0.5f;
            // float cy = fy - 0.5f;

            // float dist = sqrtf(cx*cx + cy*cy);

            // float v1 = sinf((cx * 10.0f) + XOffset * 0.01f);
            // float v2 = sinf((cy * 10.0f) + YOffset * 0.01f);
            // float v3 = sinf((dist * 20.0f) - XOffset * 0.02f);

            // float intensity = (v1 + v2 + v3) * 0.33f;
            // intensity = (intensity + 1.0f) * 0.5f;

            // uint8_t r = (uint8_t)(50  + 205 * intensity);
            // uint8_t g = (uint8_t)(20  + 100 * intensity);
            // uint8_t b = (uint8_t)(100 + 155 * intensity);

            // *Pixel = (r << 16) | (g << 8) | b;


            Pixel++;
        }

        Row += Buffer->Pitch;
    }
}

// NOTE(Tejas): DIB := Device Independent Bitmap
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {

    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width  = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFO);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->BytesPerPixel = 4;
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;

    // NOTE(Tejas): Virtual Alloc returns us Page of memory instead of the amount we asked for.
    //              so it cannot return memory less than the size of the Page. This is so that
    //              we can handle the memory allocation ourselves (Memory Pools)
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

    // TODO(Tejas): Is it better to return a newly created struct or editing struct in place using pointer?
}

// we dont make ClientRect as pointer because the compiler can inline this function.
internal void Win32DisplayBufferInWindow(HDC DeviceContext, win32_offscreen_buffer Buffer,
                                         int X, int Y, int WindowWidth, int WindowHeight) {

    // NOTE(Tejas): Adjusting the Aspect Ratio



    StretchDIBits(DeviceContext,
                  // X, Y, Width, Height,
                  // X, Y, Width, Height,
                  X, Y, WindowWidth, WindowHeight,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT WINAPI Win32MainWindowCallBack(HWND Window, UINT msg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (msg) {

    case WM_SIZE: {

        // NOTE(Tejas): we size our bitmap to match the actual size of the window,
        //              but what we want to do in the future is render our game at a
        //              fixed resolution and accomodate the window somehow.
        // win32_window_dimention Dimentions = Win32GetWindowDimention(Window);
        // Win32ResizeDIBSection(&GlobalBackbuffer, Dimentions.Width, Dimentions.Height); 
    } break;

    case WM_KEYDOWN: {
        const int velo = 1;
        if ((char)wParam == VK_UP) YOFF += velo;
        if ((char)wParam == VK_DOWN) YOFF -= velo;
        if ((char)wParam == VK_LEFT) XOFF -= velo;
        if ((char)wParam == VK_RIGHT) XOFF += velo;
    } break;

    case WM_CLOSE:
    case WM_DESTROY : {
        Running = false;
    } break;

    default: {
        result = DefWindowProc(Window, msg, wParam, lParam);
    } break;

    }

    return result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    const char* wnd_name = "Handmade-Hero";

    WNDCLASSA wc = { };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Win32MainWindowCallBack;
    wc.hInstance     = hInstance;
    wc.lpszClassName = wnd_name;

    if (RegisterClassA(&wc)) {
        
        HWND Window = CreateWindowExA(0, wc.lpszClassName, wnd_name,
                                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    NULL, NULL, hInstance, NULL);

        if (Window) {

            Win32ResizeDIBSection(&GlobalBackbuffer, 1200, 720); 

            ShowWindow(Window, nCmdShow);

            Running = true;
            while (Running) {

                MSG msg = { };
                while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {

                    if (msg.message == WM_QUIT) {
                        Running = false;
                    }

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                RenderGradient(&GlobalBackbuffer, XOFF, YOFF);

                HDC DeviceContext = GetDC(Window);

                win32_window_dimention Dimentions = Win32GetWindowDimention(Window);
                Win32DisplayBufferInWindow(DeviceContext, GlobalBackbuffer, 0, 0, Dimentions.Width, Dimentions.Height);

                ReleaseDC(Window, DeviceContext);

                XOFF++; YOFF--;
            }
            
        } else {
            
            // TODO(Tejas): Add Error handling here (failed to create a window)
        }

    } else {
        // TODO(Tejas): Add Error handling here (failed to register window class)
    }

    return 0;
}
