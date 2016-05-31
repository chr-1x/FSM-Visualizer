#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include "graphgen.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SIMULATION_ITERATIONS 1000

internal char* 
ReadFileIntoCString(char* Filename)
{
    FILE* File = fopen(Filename, "r");
    if (File != NULL) 
    {
        fseek(File, 0, SEEK_END);
        size_t Size = ftell(File);
        rewind(File);

        char* Memory = (char*)malloc(Size+1);
        size_t Result = fread((void*)Memory, 1, Size, File);
        Memory[Result] = '\0';
        if (Result == Size
            || feof(File)) 
        { 
            fclose(File);
            return Memory; 
        }
        else 
        {
            free(Memory);
            fclose(File);
            return NULL; 
        }
    }
    return NULL;
}

internal void
FixBitmap(bitmap Src, bitmap Dest)
{
    for (int Y = 0; Y < Src.Height; ++Y)
    {
        u8* SrcPixel = (u8*)Src.Memory + Y * Src.Stride;
        u8* DestPixel = (u8*)Dest.Memory + (Dest.Height - Y) * Dest.Stride;

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
    if (ArgCount < 2)
    {
        fprintf(stderr, "Usage: %s <NFAConstructorTester output file>\n", ArgValues[0]);
        return EXIT_FAILURE;
    }
    char* NFAFile = ArgValues[1];
    
    // Seed the randomness so that you can run multiple times if the
    // initial positions didn't work out well
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec);

    app_input Input = {};
    Input.SimulateOnly = true;
    Input.Mouse.P = IV2(-5000,-5000);
    Input.dt = 1.0f / 30.0f; 

    app_memory AppMemory = {};
    AppMemory.PermanentSize = Megabytes(10);
    AppMemory.TemporarySize = Megabytes(100);
    AppMemory.PermanentBlock = mmap(0, AppMemory.PermanentSize + AppMemory.TemporarySize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    AppMemory.TemporaryBlock = (u8*)AppMemory.PermanentBlock + AppMemory.PermanentSize;

    AppMemory.NFAFileCount = 1;
    AppMemory.NFAFiles[0] = ReadFileIntoCString(ArgValues[1]);

    //TODO(chronister): Paramaterize or bake into exe
    AppMemory.TTFFile = (u8*)ReadFileIntoCString("data/font.ttf");

    AppMemory.IsInitialized = true;
    
    bitmap Buffer, Buffer2;
    Buffer.Width = 1600;
    Buffer.Height = 900;
    Buffer.BytesPerPixel = 4;
    Buffer.Stride = Buffer.Width * 4;
    Buffer2 = Buffer;
    Buffer.Memory = mmap(0, Buffer.Stride*Buffer.Height, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    Buffer2.Memory = mmap(0, Buffer.Stride*Buffer.Height, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    for (int i = 0; i < SIMULATION_ITERATIONS; ++i)
    {
        if (i == SIMULATION_ITERATIONS - 1)
        {
            Input.SimulateOnly = false;
        }
        UpdateAndRender(&AppMemory, &Buffer, &Input);
    }

    FixBitmap(Buffer, Buffer2);

    int DirResult = mkdir("fsm", 0755);
    if (!(DirResult == 0 || (DirResult == -1 && errno == EEXIST)))
    {
        fprintf(stderr, "Couldn't make output directory fsm/: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    char* NFAFileName = NFAFile;
    if ((NFAFileName = index(NFAFile, '/')) != NULL)
    {
        NFAFile = NFAFileName + 1;
    }

    char* Filename;
    asprintf(&Filename, "fsm/%s.png", NFAFile);
    int ImageResult = stbi_write_png(Filename, Buffer.Width, Buffer.Height, 4, Buffer2.Memory, Buffer.Stride);
    return EXIT_SUCCESS;
}
