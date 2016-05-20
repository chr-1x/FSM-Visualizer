#pragma once
#include "chr_gamemath.h"
#include "chr_arena.h"

struct bitmap
{
    union {
        struct {
            int Width, Height;
        };
        ivec2 Dim;
    };
    int BytesPerPixel;
    void* Memory;
};

struct game_memory
{
    bool32 IsInitialized;

    u$ TemporarySize;
    void* TemporaryBlock;

    u$ PermanentSize;
    void* PermanentBlock;
};

struct game_offscreen_buffer
{
    void* Memory;
    union {
        ivec2 Dim;
        struct {
            int Width, Height;
        };
    };
    int Pitch;
};

struct game_input
{
    bool32 Reloaded;
    u64 MonotonicCounter;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_offscreen_buffer* Buffer, game_input* Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

struct game_state
{
    bool32 IsInitialized;

    int Seed;
};

struct temp_state
{
    bool32 IsInitialized;

    memory_arena TempArena;

    bitmap DragonBitmap;
};

