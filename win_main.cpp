#define NOMINMAX
#include <windows.h>

#include <stdint.h>
#include <math.h>
#include <xinput.h>
#include <dsound.h>

#include <stdio.h>

#define local static 
#define global static 
#define internal static 

#define MATH_PI 3.14159265359f

#include "handmade.cpp"

#include "win32_handmade.h"

/*
    TODO(Tejas):
    - Saved game locations
    - Getting a handle to our own executable file
    - Asset loading path
    - Threading (launch a thread)
    - Raw Input (support for multiple keyboards)
    - Sleep/timeBeginPeriod
    - ClipCursor (for multimonitor support)
    - Fullscreen support
    - WM_SETCURSOR (control cursor visibility)
    - QueryCancelAutoplay (disable autoruns)
    - WM_ACTIVATEAPP (we are not the active application)
    - Blit speed improvements
    - Hardware acceleration (OpenGL or Direct3D or both)
    - GetKeyboardLayout (for French keyboards, international WASD support)
*/

global bool GlobalRunning;
global win32_offscreen_buffer GlobalBackbuffer;

global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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

    // NOTE(Tejas): DSound uses Object Oriented COM := Component Object Model

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

            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSecondaryBuffer, 0))) {

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

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput) {

    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size, 0);

    uint8_t *DestByte = (uint8_t*)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++) {
        *DestByte++ = 0;
    }

    DestByte = (uint8_t*)Region2;
    for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++) {
        *DestByte++ = 0;
    }

    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                                   DWORD ByteToLock, DWORD BytesToWrite,
                                   game_sound_output_buffer *SoundBuffer) {

    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size, 0);
    // TODO(Tejas): Check if Region1Size and Region2Size are valid...

    DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
    int16_t *DestSample   = (int16_t*)Region1;
    int16_t *SourceSample = SoundBuffer->Samples;
    for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++) {
        *DestSample++ = *SourceSample++;
        *DestSample++ = *SourceSample++;
        SoundOutput->RunningSampleIndex++;
    }

    DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
    DestSample = (int16_t*)Region2;
    for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++) {
        *DestSample++ = *SourceSample++;
        *DestSample++ = *SourceSample++;
        SoundOutput->RunningSampleIndex++;
    }

    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

internal win32_window_dimention Win32GetWindowDimention(HWND Window) {

    win32_window_dimention Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width  = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
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

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, 
                                              game_button_state *OldState, game_button_state *NewState, 
                                              DWORD ButtonBit) {

    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
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
        uint32_t VKCode = wParam;
        bool WasDown = ((lParam & (1 << 30)) != 0);
        bool IsDown = ((lParam & (1 << 31)) == 0);
        bool AltKeyWasDown = (lParam & (1 << 29));

        int Velo = 30;
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

// NOTE(Tejas): We could add #ifs for platform specific code here like:
//              #if _WIN32
//                do windows specific stuff
//              #elif __linux__
//                do linux specific stuff
//              But this forces the control flow of every platform code to be the same.
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // NOTE(Tejas): This we need to only compute once that will give us
    //              how many counts happen per second
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);

    // NOTE(Tejas): Read MSDN for LARGE_INTEGER
    int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    WNDCLASSA wc = { };
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = Win32MainWindowCallBack;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "Handmade-Hero";

    Win32LoadXInput();
    Win32ResizeDIBSection(&GlobalBackbuffer, 1200, 720); 

    if (RegisterClassA(&wc)) {
        
        HWND Window = CreateWindowExA(0, wc.lpszClassName, wc.lpszClassName,
                                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    NULL, NULL, hInstance, NULL);

        if (Window) {

            ShowWindow(Window, nCmdShow);

            // NOTE(Tejas): if you specift the CS_OWNDC flag you can use the same DC over and over.
            //              This means, this line will only be called once at the start of the loop
            HDC DeviceContext = GetDC(Window);

            win32_sound_output SoundOutput = { };
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);

            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            // NOTE(Tejas): Get the Clock time
            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);

            uint64_t LastCycleCount = __rdtsc();

            GlobalRunning = true;
            while (GlobalRunning) {

                game_input Input[2] = { };
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

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

                // NOTE(Tejas): We are not going to think too much about Controller Input, we will just use Keyboard.
                // NOTE(Tejas): Controller Input (XUSER_MAX_COUNT := 4)
                // TODO(Tejas): Should we poll this more frequently

                int MaxControllerCount = XUSER_MAX_COUNT;
                if (MaxControllerCount > ArrayCount(NewInput->Controllers)) MaxControllerCount = ArrayCount(NewInput->Controllers);
                
                for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++) {

                    game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
                    game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

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

                        // TODO(Tejas): Do DeadZone Processing

                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Down, &NewController->Down,
                                                         XINPUT_GAMEPAD_A);
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Right, &NewController->Right,
                                                        XINPUT_GAMEPAD_B);
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Left, &NewController->Left,
                                                        XINPUT_GAMEPAD_X);
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->Up, &NewController->Up,
                                                        XINPUT_GAMEPAD_Y);
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->LeftShoulder, &NewController->LeftShoulder,
                                                        XINPUT_GAMEPAD_LEFT_SHOULDER);
                        Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                        &OldController->RightShoulder, &NewController->RightShoulder,
                                                        XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        

                        { // NOTE(Tejas): Exp code to test vibrations

                            // const float MAX_STICK = 32767.0f;
                            // const float DEADZONE = 0.1f;
                            // const float OFFSET_SPEED = 5.0f;

                            // float normX = StickX / MAX_STICK;
                            // float normY = StickY / MAX_STICK;

                            // if (fabsf(normX) < DEADZONE) normX = 0.0f;
                            // if (fabsf(normY) < DEADZONE) normY = 0.0f;

                            // normX = fminf(fmaxf(normX, -1.0f), 1.0f);
                            // normY = fminf(fmaxf(normY, -1.0f), 1.0f);

                            // float absX = fabsf(normX);
                            // float absY = fabsf(normY);

                            // WORD leftMotor  = (WORD)(absX * 5000.0f);
                            // WORD rightMotor = (WORD)(absY * 5000.0f);

                            // XINPUT_VIBRATION InputVibration = {};
                            // InputVibration.wLeftMotorSpeed  = leftMotor;
                            // InputVibration.wRightMotorSpeed = rightMotor;

                            // XInputSetState(ControllerIndex, &InputVibration);

                            // XINPUT_VIBRATION InputVibration = { };
                            // if (Left || RIGHT) InputVibration.wLeftMotorSpeed  = 100;
                            // if (Up || Down)    InputVibration.wRightMotorSpeed = 100;
                            // XInputSetState(ControllerIndex, &InputVibration);
                        }



                    } else {

                        // NOTE(Tejas): Controller at this ControllerIndex is not plugged in.
                    }
                }

                DWORD PlayCursor, WriteCursor;
                DWORD ByteToLock;
                DWORD BytesToWrite;
                bool SoundIsValid = false;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {

                    ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                    if (ByteToLock > PlayCursor) {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    } else {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }

                    SoundIsValid = true;
                }

                game_sound_output_buffer SoundBuffer = { };
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;

                game_offscreen_buffer GameBuffer = { };
                GameBuffer.Memory = GlobalBackbuffer.Memory;
                GameBuffer.Width  = GlobalBackbuffer.Width;
                GameBuffer.Height = GlobalBackbuffer.Height;
                GameBuffer.Pitch  = GlobalBackbuffer.Pitch;

                GameUpdateAndRender(&GameBuffer, &SoundBuffer, NewInput);

                if (SoundIsValid) {
                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                }

                win32_window_dimention Dimentions = Win32GetWindowDimention(Window);
                Win32DisplayBufferInWindow(DeviceContext, GlobalBackbuffer, 20, 20, Dimentions.Width, Dimentions.Height);

                // NOTE(Tejas): Counting Time Elapsed

                // NOTE(Tejas): This returns the processor time stamps. This returns the number of clock cycles
                //              since the last reset. (Read MSDN for __rdtsc)
                uint64_t EndCycleCount = __rdtsc();

                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                // NOTE(Tejas): Counting FrameTime.
                int64_t CyclesElaspsed = EndCycleCount - LastCycleCount;
                int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                float MiliSeconds = (float)(1000*CounterElapsed)/ (float)PerfCountFrequency;
                float FPS = (float)PerfCountFrequency / (float)CounterElapsed;
                float MegaCyclesPerFrame = (float)(CyclesElaspsed / (float)(1000 * 1000));

#if 0
                // FIXME(Tejas): This is Unsafe and needs to be replaced!
                char Buffer[256] = {};
                sprintf(Buffer, "%.2fms/f, %.2ff/s, %.2fmc/f\n", MiliSeconds, FPS, MegaCyclesPerFrame);
                OutputDebugStringA(Buffer);
#endif

                LastCounter    = EndCounter;
                LastCycleCount = EndCycleCount;

                game_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
                // TODO(Tejas): Should we clear these?
            }

            VirtualFree(Samples, 0, MEM_RELEASE);
            
        } else {
            
            // TODO(Tejas): Failed to create a window, Handle Error...
        }

    } else {

        // TODO(Tejas): Failed to register window class, Handle Error...
    }

    return 0;
}
