#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include "dracogen.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

internal size_t
HashString(char* String, size_t Length)
{
    size_t Hash = 0;
    for (const char* p = String; Length > 0; ++p, --Length)
    {
        Hash = 31*Hash + (int)*p;
    }
    return Hash;
}

internal void
FixBitmap(game_offscreen_buffer Src, game_offscreen_buffer Dest)
{
    for (int Y = 0; Y < Src.Height; ++Y)
    {
        u8* SrcPixel = (u8*)Src.Memory + Y * Src.Pitch;
        u8* DestPixel = (u8*)Dest.Memory + (Dest.Height - Y) * Dest.Pitch;

        for (int X = 0; X < Src.Width; ++X)
        {
            u8 R = *(SrcPixel++);
            u8 G = *(SrcPixel++);
            u8 B = *(SrcPixel++);
            u8 A = *(SrcPixel++);

            *(DestPixel++) = B;
            *(DestPixel++) = G;
            *(DestPixel++) = R;
            *(DestPixel++) = A;
        }
    }
}

int main (int ArgCount, char* ArgValues[])
{
    game_input Input;
    Input.Reloaded = false;

    game_memory GameMemory = {};
    GameMemory.PermanentSize = Megabytes(10);
    GameMemory.TemporarySize = Megabytes(100);
    GameMemory.PermanentBlock = mmap(0, GameMemory.PermanentSize + GameMemory.TemporarySize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    GameMemory.TemporaryBlock = (u8*)GameMemory.PermanentBlock + GameMemory.PermanentSize;
    GameMemory.IsInitialized = true;
    
    game_offscreen_buffer Buffer, Buffer2;
    Buffer.Width = 512;
    Buffer.Height = 512;
    Buffer.Pitch = Buffer.Width * 4;
    Buffer2 = Buffer;
    Buffer.Memory = mmap(0, Buffer.Pitch*Buffer.Height, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    Buffer2.Memory = mmap(0, Buffer.Pitch*Buffer.Height, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);


    char* DragonName = "";
    if (ArgCount < 2) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        srand(ts.tv_nsec);
        Input.MonotonicCounter = rand();
        asprintf(&DragonName, "%d", Input.MonotonicCounter);
    }
    else {
        DragonName = ArgValues[1];
        Input.MonotonicCounter = HashString(DragonName, strlen(DragonName));
    }

    GameUpdateAndRender(&GameMemory, &Buffer, &Input);

    FixBitmap(Buffer, Buffer2);

    char* Filename;
    asprintf(&Filename, "dragon_%s.png", DragonName);
    return 0 != stbi_write_png(Filename, Buffer.Width, Buffer.Height, 4, Buffer2.Memory, Buffer.Pitch);
}
