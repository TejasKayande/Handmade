#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void      *Memory;
    int        Width;
    int        Height;
    int        BytesPerPixel;
    int        Pitch;
};

struct win32_sound_output {
    uint32_t RunningSampleIndex;
    int      SamplesPerSecond;
    int      BytesPerSample;
    int      SecondaryBufferSize;
};

struct win32_window_dimention {
    int Width;
    int Height;
};

#define WIN32_HANDMADE_H
#endif // WIN32_HANDMADE_H