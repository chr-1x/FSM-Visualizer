#pragma once
#include <cstdarg>
#include "math.hpp"

template<int VertCount>
struct verts{ vec2 Verts[VertCount]; };

typedef verts<3> tri;
typedef verts<4> quad;

template<int Num>
inline verts<Num>
Vertices(vec2 First, ...)
{
    verts<Num> Result = {};
    Result.Verts[0] = First;

    va_list Args;
    va_start(Args, First);
    for (int i = 1; i < Num; ++i)
    {
        Result.Verts[i] = va_arg(Args, vec2);
    }
    va_end(Args);
    return Result;
}

template<int Num>
inline verts<Num>
operator+(verts<Num> A, vec2 Offset)
{
    verts<Num> Result = {};
    for (int VertIndex = 0; VertIndex < Num; ++VertIndex)
    {
        Result.Verts[VertIndex] = A.Verts[VertIndex] + Offset;
    }
    return Result;
}

template<int Num>
inline verts<Num>
operator-(verts<Num> A, vec2 Offset)
{
    return A + -Offset;
}

template<int Num>
inline verts<Num>
operator*(verts<Num> A, float Scale)
{
    verts<Num> Result = {};
    for (int VertIndex = 0; VertIndex < Num; ++VertIndex)
    {
        Result.Verts[VertIndex] = A.Verts[VertIndex] * Scale;
    }
    return Result;
}

template<int Num>
inline verts<Num>
operator*(verts<Num> A, vec2 Scale)
{
    verts<Num> Result = {};
    for (int VertIndex = 0; VertIndex < Num; ++VertIndex)
    {
        Result.Verts[VertIndex] = Hadamard(A.Verts[VertIndex], Scale);
    }
    return Result;
}

