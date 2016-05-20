#pragma once

struct range
{
    f32 Min, Max;    
};

// Dumb lehmer RNG
#define rM 2147483647 /* 2^31 - 1 (A large prime number) */
#define rA 16807      /* Prime root of M, passes statistical tests and produces a full cycle */
#define rQ 127773 /* M / A (To avoid overflow on A * seed) */
#define rR 2836   /* M % A (To avoid overflow on A * seed) */

#define RANDOM_MAX rM

static u32 seed = 1;

void SeedRand(int NewSeed)
{
    seed = NewSeed;
}

int Rand()
{
    uint32_t hi = seed / rQ;
    uint32_t lo = seed % rQ;
    int32_t test = rA * lo - rR * hi;
    if (test <= 0)
        test += rM;
    seed = test;
    return test;
}

inline range
BETWEEN(f32 MinVal, f32 MaxVal)
{
    range Result = { Min(MinVal, MaxVal), Max(MinVal, MaxVal) };
    return Result;
}

inline range
AROUND(f32 Center, f32 Deviation)
{
    range Result = { Center - Deviation, Center + Deviation};
    return Result;
}

internal f32
RandRange(range Range)
{
    f32 Result = ((f32)Rand() / (f32)RANDOM_MAX) * (Range.Max - Range.Min) + Range.Min;
    return Result;
}

internal f32
RandRange(f32 MinVal, f32 MaxVal)
{
    return RandRange(BETWEEN(MinVal, MaxVal));
}

internal f32
RandSign()
{
    f32 Result = Sign(RandRange(-1, 1));
    if (Result == 0.0f) {
        Result = 1.0f;
    }
    return Result;
}

internal int
RandChoice(int Range)
{
    return (Rand() % Range);
}

internal f32
Unlerp(f32 RangeMin, f32 t, f32 RangeMax)
{
    return (t - RangeMin) / (RangeMax - RangeMin);
}

internal f32
RemapRange(f32 Val, f32 SourceMin, f32 SourceMax, f32 DestMin, f32 DestMax)
{
    return Lerp(DestMin, Unlerp(SourceMin, Val, SourceMax), DestMax);
}

internal f32
ModRange(f32 Val, f32 Max)
{
    while (Val < 0.0f) { Val += Max; }
    while (Val >= Max) { Val -= Max; }
    return Val;
}

