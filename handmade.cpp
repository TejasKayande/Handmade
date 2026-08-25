
#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz) {

    static float tSine;

    int16_t ToneVolume = 3000;
    int16_t *SampleOut = SoundBuffer->Samples;

    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++) {

        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * (float)MATH_PI * 1.0f / (float)WavePeriod;
    }
}

internal void RenderWeirdGradient(game_offscreen_buffer *GameBuffer, int XOffset, int YOffset) {

    // TODO(Tejas): What is better pass by reference or pass by value?

    uint8_t *Row = (uint8_t*) GameBuffer->Memory;

    for (int Y = 0; Y < GameBuffer->Height; Y++) {

        uint32_t *Pixel = (uint32_t*) Row;

        for (int X = 0; X < GameBuffer->Width; X++) {
            // AA RR GG BB 

            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset);

            *Pixel++ = (Green << 8) | Blue;
        }

        Row += GameBuffer->Pitch;
    }
}

void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input) {

    local int BlueOffset  = 0;
    local int GreenOffset = 0;
    local int ToneHz = 256;

    game_controller_input *Input0 = &Input->Controllers[0];
    if (Input0->Up.EndedDown) {
        ToneHz = 256 + (int)(128.0f * Input0->EndX);
        BlueOffset += (int)(4.0f * (Input0->EndY));
    } else {

    }

    if (Input0->Down.EndedDown) {
        ToneHz = 256 + (int)(128.0f * Input0->EndX);
        BlueOffset -= (int)(4.0f * (Input0->EndY));
    } else {

    }

    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
