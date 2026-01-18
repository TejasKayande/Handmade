#define NOMINMAX
#include <windows.h>

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (msg) {

    case WM_QUIT:
    case WM_DESTROY : {
        PostQuitMessage(0);
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        LONG X      = ps.rcPaint.left;
        LONG Y      = ps.rcPaint.top;
        LONG height = ps.rcPaint.bottom - ps.rcPaint.top;
        LONG width  = ps.rcPaint.right - ps.rcPaint.left;

        static DWORD op = WHITENESS;
        PatBlt(hdc, 0, 0, width, height, op);
        op = (op == WHITENESS) ? BLACKNESS : WHITENESS;
        EndPaint(hwnd, &ps);
    }

    default: {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
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
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = wnd_name;

    if (RegisterClassA(&wc)) {
        
        HWND hwnd = CreateWindowExA(0, wc.lpszClassName, wnd_name,
                                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                    (int)wnd_width, (int)wnd_height,
                                    NULL, NULL, hInstance, NULL);

        if (hwnd) {

            ShowWindow(hwnd, nCmdShow);

            MSG msg = { };
            for (;;) {
                BOOL rst = GetMessage(&msg, NULL, 0, 0);

                if (rst > 0) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                else {
                    break;
                }

            }
            
        } else {
            
            // TODO(Tejas): Add Error handling here (failed to create a window)
        }

    } else {
        // TODO(Tejas): Add Error handling here (failed to register window class)
    }

    return 0;
}
