
#if !defined(HANDMADE_H)

/*
    NOTE(Tejas):

    HANDMADE_INTERNAL:
        0 - Build for public release
        1 - Build for developer only

    HANDMADE_SLOW:
        0 - No slow code allowed!
        1 - Slow code welcome.
*/

#if HANDMADE_SLOW 
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) ((Value) * 1024LL * 1024LL)
#define Gigabytes(Value) ((Value) * 1024LL * 1024LL * 1024LL)
#define Terabytes(Value) ((Value) * 1024LL * 1024LL * 1024LL * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline int32_t SafeTruncateUInt64(uint64_t Value) {
    Assert(Value <= 0xFFFFFFFF);
    int32_t Result = (int32_t)Value;
    return Result;
}

// NOTE(Tejas): Services that the platform layer provides to the game
#if HANDMADE_INTERNAL
// IMPORTANT(Tejas): These are not to be used in shipping code.
struct debug_read_file_result {
    uint32_t ContentsSize;
    void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory);
#endif

// TODO(Tejas): Swap, Min, Max... Macros???

// NOTE(Tejas): There are 2 approaches you can take when seperating platform and game:

// 1. Platform as a Service to the game:

// NOTE(Tejas): now the platform_window will be defined in the platform
//              specific code, so in win32_main.cpp or linux_main.cpp, where we
//              can adjust what we actually want in that struct that is
//              platform specific, but to this layer it is completely opaque
//              and we can just use it as a pointer to the platform_window
//              struct, and we can use it in the platform specific code to
//              access the members of the struct that are platform specific.
// struct platform_window;
// platform_window *PlatformCreateWindow(int Width, int Height, const char *Title);
// void PlatformCloseWindow(platform_window *Window);

// 2. Game as a Service to the platform:
struct game_offscreen_buffer {
    void   *Memory;
    int     Width;
    int     Height;
    int     Pitch;
};

struct game_sound_output_buffer {
    int      SamplesPerSecond;
    int      SampleCount;
    int16_t *Samples;
};

struct game_button_state {
    int HalfTransitionCount;
    bool EndedDown;
};

struct game_controller_input {

    bool IsAnalog;

    float StartX;
    float StartY;

    float MinX;
    float MinY;

    float MaxX;
    float MaxY;

    float EndX;
    float EndY;

    union {
        game_button_state Buttons[6];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input {
    // TODO(Tejas): Pass clock values here
    game_controller_input Controllers[4];
};

struct game_state {
    int ToneHz;
    int BlueOffset;
    int GreenOffset;
};

struct game_memory {

    bool IsInitialized;

    uint64_t PermanentStorageSize;
    void *PermanentStorage; // NOTE(Tejas): Required to be cleared to zero at startup

    uint64_t TransientStorageSize;
    void *TransientStorage; // NOTE(Tejas): Required to be cleared to zero at startup
};

void GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input);

#define HANDMADE_H
#endif