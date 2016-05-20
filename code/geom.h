#pragma once

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
VerticesByOffsets(vec2 First, ...)
{
    verts<Num> Result = {};
    Result.Verts[0] = First;

    vec2 Last = First;
    va_list Args;
    va_start(Args, First);
    for (int i = 1; i < Num; ++i)
    {
        Result.Verts[i] = Last + va_arg(Args, vec2);
        Last = Result.Verts[i];
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
RotateAroundOrigin(verts<Num> A, float Angle)
{
    verts<Num> Result = {};
    mat3 RotationMat = RotationMatrix(Angle);
    for (int VertIndex = 0; VertIndex < Num; ++VertIndex)
    {
        Result.Verts[VertIndex] = (RotationMat * V3(A.Verts[VertIndex], 0)).xy;
    }
    return Result;
}

template<int Num>
inline verts<Num>
LineOffsetParametric(verts<Num> A, vec2 (*AmountToAdd)(f32 Percent))
{
    verts<Num> Result = {};
    f32 TotalLength = 0.0f;
    for (int VertIndex = 1; VertIndex < Num; ++VertIndex)
    {
        TotalLength += Length(A.Verts[VertIndex] - A.Verts[VertIndex - 1]);
    }

    f32 Percent = 0.0f;
    for (int VertIndex = 0; VertIndex < Num; ++VertIndex)
    {
        if (VertIndex > 0) {
            f32 SegmentLen = Length(A.Verts[VertIndex] - A.Verts[VertIndex - 1]);
            Percent += SegmentLen / TotalLength;
        }
        Result.Verts[VertIndex] = A.Verts[VertIndex] + AmountToAdd(Percent);
    }
    return Result;
}

template<int LineVertCount, int PolyVertCount>
inline verts<PolyVertCount>
LineTransformParametric(verts<LineVertCount> Line, verts<PolyVertCount> Poly, f32 TargetPercent, 
                        verts<PolyVertCount> (*TransformFunc)(verts<PolyVertCount> Poly, f32 Percent, vec2 LineDir, vec2 LinePos))
{
    verts<PolyVertCount> Result = {};
    f32 TotalLength = 0.0f;
    for (int VertIndex = 1; VertIndex < LineVertCount; ++VertIndex)
    {
        TotalLength += Length(Line.Verts[VertIndex] - Line.Verts[VertIndex - 1]);
    }

    f32 Percent = 0.0f;
    for (int VertIndex = 1; VertIndex < LineVertCount; ++VertIndex)
    {
        f32 SegmentLen = Length(Line.Verts[VertIndex] - Line.Verts[VertIndex - 1]);
        f32 NextPercent = Percent + SegmentLen / TotalLength;

        if (Percent <= TargetPercent && TargetPercent < NextPercent) {
            vec2 Direction = Line.Verts[VertIndex] - Line.Verts[VertIndex - 1];
            Result = TransformFunc(Poly, TargetPercent, Direction, 
                                   Line.Verts[VertIndex - 1] + Direction * Unlerp(Percent, TargetPercent, NextPercent));
            break;
        }

        Percent = NextPercent;
    }
    return Result;
}

