/* memory_arena.hpp
 * based on a similar data structure from Handmade Hero, by Casey Muratori
 *
 * Implements a simple stack-based memory allocation structure.
 * Typical use cases:

 *  - You want to store some intermediate calculations during processing that
 *    don't fit in a single type.  Start a temporary_memory using BeginTemporaryMemory 
 *    and push everything you need onto the memory arena. When done with
 *    processing, simply EndTemporaryMemory to bump the arena back to where it
 *    started.
 *  - You have some unstructured data that needs to be stored for the duration
 *    of the lifetime of the application. Push it onto the arena when it's created
 *    and don't worry about it after that.
 *
 *  What it is NOT:
 *   - A general purpose allocator. Free is all-or-nothing, at the granularity
 *     level of temporary_memory structs.
 *   - Safe in any meaningful way. Data appears consecutively and bad writes can
 *     overwrite later data. Use discretion.
 *
 *  The benefits are that it is fast and extremely simple to use. You can start
 *  an arena in any opaque block of memory and freeing is essentially a no-op. 
 *
 *  Since most tasks lend themselves well to stack-based memory use patterns, it
 *  is sufficient for a lot of cases. The most notable exception is string processing,
 *  for which pools or heap allocators tend to be more natural solutions.
 */

#pragma once

// Purpose: asserting upon error conditions
#include <cassert>

/* Convenience typedef to give a bit more descriptive name to the use of size_t
 * here. */
typedef size_t memory_index;

/* Main structure which describes a memory arena. */
struct memory_arena
{
    // The total size, in bytes, of the block this arena can allocate out of.
    memory_index Size;
    // A pointer to the start of the block this arena will allocate out of.
    u8* Base;
    // The total number of bytes of the arena which have been filled.
    memory_index Used;
    // The number of temporary_memory blocks which have been taken out of the
    // arena.
	s32 TempCount;
};

/* Structure describing a snapshot of the state of the arena at a certain
 * moment, which can be restored once computation is complete. */
struct temporary_memory
{
    // The arena this snapshot was taken of
	memory_arena* Arena;
    // How much was used by this snapshot
	memory_index Used;
};

/* Initializes a memory arena on a memory block of the given Size starting at
 * Base. */
inline void
InitializeArena(memory_arena *Arena, memory_index Size, void* Base)
{
    Arena->Size = Size;
    Arena->Base = (u8*)Base;
    Arena->Used = 0;
	Arena->TempCount = 0;
}


/* Macros which ease the process of using the arena by allowing you to directly
 * push on values the size of a struct, array, or size in bytes. */
#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)

/* Returns a pointer to the start of a block of Size bytes which has been set
 * aside out of the arena. */
inline void* 
PushSize_(memory_arena *Arena, memory_index Size)
{
    assert(Arena->Used + Size <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return(Result);
}

/* Begins a temporary_memory transaction, returning a struct representing the
 * state of the memory at the start of the transaction. */
inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena)
{
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->Used;
	++Arena->TempCount;

	return Result;
}

/* Ends a temporary_memory transaction, restoring the memory arena to the state
 * it was at when BeginTemporaryMemory was called. */
inline void
EndTemporaryMemory(temporary_memory TempMem)
{
	memory_arena* Arena = TempMem.Arena;
	assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	assert(Arena->TempCount > 0);
	--Arena->TempCount;
}
