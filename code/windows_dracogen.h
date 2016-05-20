#pragma once

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width; int Height;
    int Pitch;
};

struct win32_game_code
{
    bool32 IsValid;
    HMODULE DLLHandle;
    FILETIME DLLLastWriteTime;

    game_update_and_render* UpdateAndRender;
};
