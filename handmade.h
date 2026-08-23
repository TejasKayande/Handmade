
#if !defined(HANDMADE_H)

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

void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif