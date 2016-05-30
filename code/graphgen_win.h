#pragma once

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width; int Height;
    int Pitch;
};

struct win32_dynamic_code
{
    bool IsValid;
    HMODULE DLLHandle;
    FILETIME DLLLastWriteTime;

    update_and_render* UpdateAndRender;
};
