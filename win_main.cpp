#define NOMINMAX
#include <windows.h>

#include <stdint.h>
#include <math.h>
#include <xinput.h>
#include <dsound.h>

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

global bool GlobalRunning;
global win32_offscreen_buffer GlobalBackbuffer;

global unsigned int XOFF, YOFF;

// NOTE(Tejas): We Load the XInput and DirectSound DLL ourselves so that we know it exists.
//              if it does not exist, then we can just ignore it so the user
//              is free to use any other input API that is supported and can play without sounds.

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }

// NOTE(Tejas): we can check if the function is Stub to check its validity
global x_input_get_state *XInputGetState_ = XInputGetStateStub; 
global x_input_set_state *XInputSetState_ = XInputSetStateStub;

// NOTE(Tejas): This just shadows the defination of XInputGetState and XInputSetState
//              that is coming from xinput.h
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
DIRECT_SOUND_CREATE(DirectSoundCreateStub) { return (DSERR_NODRIVER); }
global direct_sound_create *DirectSoundCreate_ = DirectSoundCreateStub;
#define DirectSoundCreate DirectSoundCreate_

internal void Win32LoadXInput(void) {

    // NOTE(Tejas): Loding the XInput functions that we need form xinput.dll
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

    // NOTE(Tejas): If Loading 1.4 fails, Load 1.3
    if (!XInputLibrary) XInputLibrary = LoadLibraryA("xinput1_3.dll");

    if (XInputLibrary) {
        // TODO(Tejas): We should probably check if GetProcAddress actually
        //              returns a valid pointer if not we have to set these back
        //              to Stub. But if we manage to load the dll but the
        //              function does not exist, we cant relie on the to work
        //              properly anyways.

        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");

    } else {
        // TODO(Tejas): Couldnt Load the Library, Handle Error...
    }
}

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {

    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary) {

        DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound,0))) {

            WAVEFORMATEX WaveFormat = { };
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2; // NOTE(Tejas): 2 for stereo
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            // NOTE(Tejas): Read MSDN for IDirectSound8
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {

                // NOTE(Tejas): MSDN docs suggest that this struct needs to be zero'd out...
                DSBUFFERDESC BufferDesc = {  };
                BufferDesc.dwSize = sizeof(BufferDesc);
                BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0))) {

                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
                        OutputDebugStringA("Primary BUffer Created\n");
                    } else {
                        // TODO(Tejas): Couldnt set the Primary Buffer Format, Handle Error...
                    }

                } else {
                    
                    // TODO(Tejas): Couldnt Create the Direct Sound Buffer, Handle Error...
                }
                
            } else {
                // TODO(Tejas): Couldnt set the Cooperative Level, Handle Error...
            }

            // CREATE SECONDARY BUFFER

            DSBUFFERDESC BufferDesc = { };
            BufferDesc.dwSize = sizeof(BufferDesc);
            BufferDesc.dwBufferBytes = BufferSize;
            BufferDesc.lpwfxFormat = &WaveFormat;

            LPDIRECTSOUNDBUFFER SecondaryBuffer;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &SecondaryBuffer, 0))) {

                int flag = 4 + 8;
                OutputDebugStringA("Secondary BUffer Created\n");
                
            } else {
                
                // TODO(Tejas): Couldnt set the Secondary Buffer Format, Handle Error...
            }

        } else {
            // TODO(Tejas): Couldnt Load the Function from dll, Handle Error...
        }
        
    } else {
        // TODO(Tejas): Couldnt Load the Library, Handle Error...
    }
}

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

            *Pixel++ = (Green << 8) | Blue;
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

    // NOTE(Tejas): Buffer->Height is -ve here is so that windows treats it as top down and
    //              not bottom up.
    Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFO);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
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

    StretchDIBits(DeviceContext,
                  // X, Y, Width, Height,
                  // X, Y, Width, Height,
                  // X, Y, WindowWidth, WindowHeight,
                  X, Y, Buffer.Width, Buffer.Height,
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

    // TODO(Tejas): There are more keyboard events, look into those
    // NOTE(Tejas): WM_SYSKEYDOWN and WM_SYSKEYUP means we have to handle alt key keybinds,
    //              Alt-f4 doesnt work with these.
    case WM_CHAR:
    case WM_KEYUP:
    case WM_KEYDOWN: {
        const int velo = 5;
        if ((char)wParam == 'W') YOFF += velo;
        if ((char)wParam == 'S') YOFF -= velo;
        if ((char)wParam == 'D') XOFF -= velo;
        if ((char)wParam == 'A') XOFF += velo;
    } break;

    case WM_CLOSE:
    case WM_DESTROY : {
        GlobalRunning = false;
    } break;

    case WM_PAINT: {

        PAINTSTRUCT p;
        HDC DeviceContext = BeginPaint(Window, &p);

        RECT rect;
        GetClientRect(Window, &rect);
        HBRUSH color = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(DeviceContext, &rect, color);
        DeleteObject(color);

        EndPaint(Window, &p);
        
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
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = Win32MainWindowCallBack;
    wc.hInstance     = hInstance;
    wc.lpszClassName = wnd_name;

    Win32LoadXInput();
    Win32ResizeDIBSection(&GlobalBackbuffer, 1200, 720); 

    if (RegisterClassA(&wc)) {
        
        HWND Window = CreateWindowExA(0, wc.lpszClassName, wnd_name,
                                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    NULL, NULL, hInstance, NULL);

        if (Window) {

            ShowWindow(Window, nCmdShow);

            Win32InitDSound(Window, 48000, 48000*sizeof(int16_t)*2);


            // NOTE(Tejas): if you specift the CS_OWNDC flag you can use the same DC over and over.
            //              This means, this line will only be called once at the start of the loop
            HDC DeviceContext = GetDC(Window);

            GlobalRunning = true;
            while (GlobalRunning) {

                MSG msg = { };
                while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {

                    if (msg.message == WM_QUIT) {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                // NOTE(Tejas): XInputGetState will stall for a couple of miliseconds if it cant find a
                //              connected controller. We have to only check input for the controllers that
                //              we know are connected.

                // NOTE(Tejas): Controller Input (XUSER_MAX_COUNT := 4)
                // TODO(Tejas): Should we poll this more frequently
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++) {

                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {

                        // NOTE(Tejas): See XINPUT_GAMEPAD Defination...

                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up    = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down  = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left  = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back  = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        // TODO(Tejas): I dont know how the values are calculate for the stick movement
                        int16_t StickX = Pad->sThumbLX;
                        int16_t StickY = Pad->sThumbLY;

                        int velo = 10;
                        if (Up)    YOFF -= velo;
                        if (Down)  YOFF += velo;
                        if (Right) XOFF += velo;
                        if (Left)  XOFF -= velo;

                        const float MAX_STICK = 32767.0f;
                        const float DEADZONE = 0.1f;
                        const float OFFSET_SPEED = 5.0f;

                        float normX = StickX / MAX_STICK;
                        float normY = StickY / MAX_STICK;

                        if (fabsf(normX) < DEADZONE) normX = 0.0f;
                        if (fabsf(normY) < DEADZONE) normY = 0.0f;

                        normX = fminf(fmaxf(normX, -1.0f), 1.0f);
                        normY = fminf(fmaxf(normY, -1.0f), 1.0f);

                        float absX = fabsf(normX);
                        float absY = fabsf(normY);

                        WORD leftMotor  = (WORD)(absX * 5000.0f);
                        WORD rightMotor = (WORD)(absY * 5000.0f);

                        XOFF += leftMotor % 20;
                        YOFF += rightMotor % 20;

                        XINPUT_VIBRATION InputVibration = {};
                        InputVibration.wLeftMotorSpeed  = leftMotor;
                        InputVibration.wRightMotorSpeed = rightMotor;

                        XInputSetState(ControllerIndex, &InputVibration);


                        // XINPUT_VIBRATION InputVibration = { };
                        // if (Left || RIGHT) InputVibration.wLeftMotorSpeed  = 100;
                        // if (Up || Down)    InputVibration.wRightMotorSpeed = 100;
                        // XInputSetState(ControllerIndex, &InputVibration);

                    } else {

                        // NOTE(Tejas): Controller at this ControllerIndex is not plugged in.
                    }
                }

                RenderGradient(&GlobalBackbuffer, XOFF, YOFF);

                win32_window_dimention Dimentions = Win32GetWindowDimention(Window);
                Win32DisplayBufferInWindow(DeviceContext, GlobalBackbuffer, 20, 20, Dimentions.Width, Dimentions.Height);
            }
            
        } else {
            
            // TODO(Tejas): Failed to create a window, Handle Error...
        }

    } else {

        // TODO(Tejas): Failed to register window class, Handle Error...
    }

    return 0;
}
