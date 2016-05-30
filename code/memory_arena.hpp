

#pragma once
#include <cassert>

typedef size_t memory_index;

struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;
	int32 TempCount;
};

struct temporary_memory
{
	memory_arena* Arena;
	memory_index Used;
};

inline void
InitializeArena(memory_arena *Arena, memory_index Size, void* Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8*)Base;
    Arena->Used = 0;
	Arena->TempCount = 0;
}

inline memory_arena
AllocateArena(memory_index Size, void* Block)
{
    memory_arena Result = {};
    InitializeArena(&Result, Size, Block);
    return Result;
}

inline memory_index
GetAlignmentOffset(memory_arena* Arena, memory_index Alignment)
{
	memory_index AlignmentOffset = 0;
	memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
	memory_index AlignmentMask = Alignment - 1;
	if (ResultPointer & AlignmentMask)
	{
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}

	return AlignmentOffset;
}

inline memory_index
GetArenaSizeRemaining(memory_arena* Arena, memory_index Alignment = 4)
{
	memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));
	return Result;
}

#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)

inline void* 
PushSize_(memory_arena *Arena, memory_index Size)
{
    assert(Arena->Used + Size <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return(Result);
}

inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena)
{
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->Used;
	++Arena->TempCount;

	return Result;
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
	memory_arena* Arena = TempMem.Arena;
	assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	assert(Arena->TempCount > 0);
	--Arena->TempCount;
}

inline void
CheckArena(memory_arena* Arena)
{
	assert(Arena->TempCount == 0);
}
