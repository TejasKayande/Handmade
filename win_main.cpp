#define NOMINMAX
#include <windows.h>

#include <stdint.h>

#define local static 
#define global static 
#define internal static 

global bool Running;

global BITMAPINFO BitmapInfo;
global void *BitmapMemory;

global int BytesPerPixel;
global int BitmapWidth;
global int BitmapHeight;

global int XOFF, YOFF;

internal void RenderGradient(int XOffset, int YOffset) {

    int Width = BitmapWidth;
    int Height = BitmapHeight;
    
    int Pitch = Width * BytesPerPixel;
    uint8_t *Row = (uint8_t*) BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; Y++) {
        uint32_t *Pixel = (uint32_t*) Row;
        for (int X = 0; X < BitmapWidth; X++) {
            // AA RR GG BB 
            *Pixel = (0x0 << 24) | (0x0 << 16) | ((X + XOffset) << 8) | (Y + YOffset);
            Pixel++;
        }

        Row += Pitch;
    }
}

// NOTE(Tejas): DIB := Device Independent Bitmap
internal void Win32ResizeDIBSection(int Width, int Height) {

    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;


    BytesPerPixel = 4;
    int BitmapMemorySize = (Width * Height) * BytesPerPixel;

    // NOTE(Tejas): Virtual Alloc returns us Page of memory instead of the amount we asked for.
    //              so it cannot return memory less than the size of the Page. This is so that
    //              we can handle the memory allocation ourselves (Memory Pools)
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height) {

    int WindowWidth  = ClientRect->right  - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    StretchDIBits(DeviceContext,
                  // X, Y, Width, Height,
                  // X, Y, Width, Height,
                  0, 0, BitmapWidth, BitmapHeight,
                  0, 0, WindowWidth, WindowHeight,
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT WINAPI Win32MainWindowCallBack(HWND Window, UINT msg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (msg) {

    case WM_SIZE: {

        // NOTE(Tejas): we size our bitmap to match the actual size of the window,
        //              but what we want to do in the future is render our game at a
        //              fixed resolution and accomodate the window somehow.
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        LONG Width  = ClientRect.right - ClientRect.left;
        LONG Height = ClientRect.bottom - ClientRect.top;
        Win32ResizeDIBSection(Width, Height); 
    } break;

    case WM_KEYDOWN: {
        const int velo = 100;
        if ((char)wParam == 'W') YOFF += velo;
        if ((char)wParam == 'S') YOFF -= velo;
        if ((char)wParam == 'A') XOFF -= velo;
        if ((char)wParam == 'D') XOFF += velo;
    } break;

    case WM_CLOSE:
    case WM_DESTROY : {
        Running = false;
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC DeviceContext = BeginPaint(Window, &ps);

        LONG X      = ps.rcPaint.left;
        LONG Y      = ps.rcPaint.top;
        LONG Width  = ps.rcPaint.right - ps.rcPaint.left;
        LONG Height = ps.rcPaint.bottom - ps.rcPaint.top;

        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);

        EndPaint(Window, &ps);
    }

    default: {
        result = DefWindowProc(Window, msg, wParam, lParam);
    } break;

    }

    return result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    int wnd_width  = 1000;
    int wnd_height = 800;

    const char* wnd_name = "Handmade-Hero";

    WNDCLASSA wc = { };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Win32MainWindowCallBack;
    wc.hInstance     = hInstance;
    wc.lpszClassName = wnd_name;

    if (RegisterClassA(&wc)) {
        
        HWND Window = CreateWindowExA(0, wc.lpszClassName, wnd_name,
                                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                    (int)wnd_width, (int)wnd_height,
                                    NULL, NULL, hInstance, NULL);

        if (Window) {

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

                RenderGradient(XOFF, YOFF);

                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);

                ReleaseDC(Window, DeviceContext);
            }
            
        } else {
            
            // TODO(Tejas): Add Error handling here (failed to create a window)
        }

    } else {
        // TODO(Tejas): Add Error handling here (failed to register window class)
    }

    return 0;
}
