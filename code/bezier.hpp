#pragma once
#include "math.hpp"
#include <cstdarg>

inline vec2
BezierQuadratic(vec2 P0, vec2 P1, vec2 P2, f32 t)
{
    return P0 * Square(1 - t) + P1 * 2*(1 - t)*t + Square(t)*P2;
}

inline vec2
BezierQuadraticDerivative(vec2 P0, vec2 P1, vec2 P2, f32 t)
{
    return (P1 - P0) * 2*(1 - t) + (P2 - P1) * 2*t;
}

inline vec2
BezierCubic(vec2 P0, vec2 P1, vec2 P2, vec2 P3, f32 t)
{
    return (1.0f - t)*BezierQuadratic(P0, P1, P2, t) + t*BezierQuadratic(P1, P2, P3, t);
}

struct bezier_quadratic_segment 
{
    vec2 StartP;  
    vec2 ControlP;   
    vec2 EndP;
};

template <int SegmentCount>
struct bezier_quadratic
{
    bezier_quadratic_segment Segments[SegmentCount];

    //NOTE(chronister): These are APPROXIMATES!
    // The approximation accuracy is determined by the argument
    // passed to ApproximateBezierQuadraticLength in 
    // the Curve construction functions
    f32 Lengths[SegmentCount];
    f32 TotalLength;
};

struct bezier_cubic_segment 
{
    vec2 StartP;  
    vec2 ControlP1;
    vec2 ControlP2;   
    vec2 EndP;
};

template <int SegmentCount>
struct bezier_cubic
{
    bezier_cubic_segment Segments[SegmentCount];

    //NOTE(chronister): These are APPROXIMATES!
    // The approximation accuracy is determined by the argument
    // passed to ApproximateBezierQuadraticLength in 
    // the Curve construction functions
    f32 Lengths[SegmentCount];
    f32 TotalLength;
};

inline vec4
BezierQuadraticInverse(vec2 P0, vec2 P1, vec2 P2, vec2 X)
{
    // NOTE(chronister): Only applies if P0 - 2P1 + P2 ≠ 0
    vec2 Radical = -2*Hadamard(P1,X) + Hadamard(P0,X) + Hadamard(P2,X) + Hadamard(P1,P1)-Hadamard(P0,P2);
    vec2 Denominator = P0 - 2*P1 + P2;
    vec2 A = P0 - P1;
    vec4 t =   { (A.x + sqrtf(Radical.x))/Denominator.x,
                 (A.x - sqrtf(Radical.x))/Denominator.x,
                 (A.y + sqrtf(Radical.y))/Denominator.y,
                 (A.y - sqrtf(Radical.y))/Denominator.y };
    return t;
}

inline f32
ApproximateBezierQuadraticLength(vec2 StartP, vec2 ControlP, vec2 EndP, int ApproximationSteps)
{
    f32 LengthSoFar = 0.0f;
    vec2 TestP = StartP;
    for (int i = 1; i <= ApproximationSteps; ++i)
    {
        vec2 NextTestP = BezierQuadratic(StartP, ControlP, EndP, (f32)i / (f32)ApproximationSteps);
        LengthSoFar += Length(NextTestP - TestP);
        TestP = NextTestP;
    }
    return LengthSoFar;
}

inline f32
ApproximateBezierCubicLength(vec2 StartP, vec2 ControlP1, vec2 ControlP2, vec2 EndP, int ApproximationSteps)
{
    f32 LengthSoFar = 0.0f;
    vec2 TestP = StartP;
    for (int i = 1; i <= ApproximationSteps; ++i)
    {
        vec2 NextTestP = BezierCubic(StartP, ControlP1, ControlP2, EndP, (f32)i / (f32)ApproximationSteps);
        LengthSoFar += Length(NextTestP - TestP);
        TestP = NextTestP;
    }
    return LengthSoFar;
}

/* NOTE(chronister):
 * This function will only work properly if given an even number of varargs,
 * where each pair of args is:
 *  ControlP NextP
 * and the previous point is used as the starting point of the curve.
 * It uses the template argument to deduce how many triplets of vertices to produce,
 *  reading 2(Num - 1) arguments total */
template<int SegmentCount>
inline bezier_quadratic<SegmentCount>
CurveQuadratic(vec2 FirstP, vec2 ControlP, vec2 NextP ...)
{
    bezier_quadratic<SegmentCount> Result = {};

    f32 TotalLength = 0.0f;

    va_list Args;
    va_start(Args, NextP);
    for (int i = 0; i < SegmentCount; ++i)
    {
        Result.Segments[i].StartP = FirstP;
        Result.Segments[i].ControlP = ControlP;
        Result.Segments[i].EndP = NextP;

        Result.Lengths[i] = ApproximateBezierQuadraticLength(FirstP, ControlP, NextP, 20);
        TotalLength += Result.Lengths[i];

        if (i < SegmentCount - 1) {
            FirstP = NextP;
            ControlP = va_arg(Args, vec2);
            NextP = va_arg(Args, vec2);
        }
    }

    Result.TotalLength = TotalLength;

    return Result;
}

/* NOTE(chronister):
 * This function will only work properly if given an odd number of varargs,
 * where each pair of args is:
 *  ControlP1 ControlP2 NextP
 * and the previous point is used as the starting point of the curve.
 * It uses the template argument to deduce how many triplets of vertices to produce,
 *  reading 3(Num - 1) arguments total */
template<int SegmentCount>
inline bezier_cubic<SegmentCount>
CurveCubic(vec2 FirstP, vec2 ControlP1, vec2 ControlP2, vec2 NextP ...)
{
    bezier_cubic<SegmentCount> Result = {};

    f32 TotalLength = 0.0f;

    va_list Args;
    va_start(Args, NextP);
    for (int i = 0; i < SegmentCount; ++i)
    {
        Result.Segments[i].StartP = FirstP;
        Result.Segments[i].ControlP1 = ControlP1;
        Result.Segments[i].ControlP2 = ControlP2;
        Result.Segments[i].EndP = NextP;

        Result.Lengths[i] = ApproximateBezierCubicLength(FirstP, ControlP1, ControlP2, NextP, 20);
        TotalLength += Result.Lengths[i];

        if (i < SegmentCount - 1) {
            FirstP = NextP;
            ControlP1 = va_arg(Args, vec2);
            ControlP2 = va_arg(Args, vec2);
            NextP = va_arg(Args, vec2);
        }
    }

    Result.TotalLength = TotalLength;

    return Result;
}
