#pragma once
#include "geom.h"

template <int SegmentCount>
struct bezier_curve
{
    struct bezier_quadratic {
        vec2 StartP;  
        vec2 ControlP;   
        vec2 EndP;
    } Segments[SegmentCount];

    //NOTE(chronister): These are APPROXIMATES!
    // The approximation accuracy is determined by the argument
    // passed to ApproximateBezierQuadraticLength in 
    // the Curve construction functions
    f32 Lengths[SegmentCount];
    f32 TotalLength;
};

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

inline f32
ApproximateBezierQuadraticLength(vec2 StartP, vec2 ControlP, vec2 EndP, int ApproximationSteps = 10)
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

/* NOTE(chronister):
 * These functions will only work properly if given an even number of varargs,
 * where each pair of args is:
 *  ControlP NextP
 * and the previous point is used as the starting point of the curve.
 * It uses the template argument to deduce how many triplets of vertices to produce,
 *  reading 2(Num - 1) arguments total */
template<int SegmentCount>
inline bezier_curve<SegmentCount>
Curve(vec2 FirstP, vec2 ControlP, vec2 NextP ...)
{
    bezier_curve<SegmentCount> Result = {};

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

        if (i < Num - 1) {
            FirstP = NextP;
            ControlP = va_arg(Args, vec2);
            NextP = va_arg(Args, vec2);
        }
    }

    return Result;
}

// Same as above but each point is specified relative to the previous
template<int SegmentCount>
inline bezier_curve<SegmentCount>
CurveByOffsets(vec2 FirstP, vec2 ControlP, vec2 NextP ...)
{
    bezier_curve<SegmentCount> Result = {};

    vec2 BaseP = V2(0,0);

    va_list Args;
    va_start(Args, NextP);
    for (int i = 0; i < SegmentCount; ++i)
    {
        Result.Segments[i].StartP = (BaseP = FirstP);
        Result.Segments[i].ControlP = (BaseP += ControlP);
        Result.Segments[i].EndP = (BaseP += NextP);

        Result.Lengths[i] = ApproximateBezierQuadraticLength(Result.Segments[i].StartP, 
                                                             Result.Segments[i].ControlP, 
                                                             Result.Segments[i].EndP, 20);
        Result.TotalLength += Result.Lengths[i];

        if (i < SegmentCount - 1) {
            FirstP = BaseP;
            ControlP = va_arg(Args, vec2);
            NextP = va_arg(Args, vec2);
        }
    }

    return Result;
}

template<int SegmentCount, int PolyVertCount>
inline verts<PolyVertCount>
CurveTransformParametric(bezier_curve<SegmentCount> Curve, verts<PolyVertCount> Poly, f32 TargetPercent, 
                        verts<PolyVertCount> (*TransformFunc)(verts<PolyVertCount> Poly, f32 Percent, vec2 Dir, vec2 Pos))
{
    verts<PolyVertCount> Result = {};

    f32 Percent = 0.0f;
    for (int SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        f32 NextPercent = Percent + (Curve.Lengths[SegmentIndex] / Curve.TotalLength);

        if (Percent <= TargetPercent && TargetPercent < NextPercent) {
            vec2 Position = BezierQuadratic(Curve.Segments[SegmentIndex].StartP,
                                            Curve.Segments[SegmentIndex].ControlP,
                                            Curve.Segments[SegmentIndex].EndP,
                                            Unlerp(Percent, TargetPercent, NextPercent));

            vec2 Direction = BezierQuadraticDerivative(Curve.Segments[SegmentIndex].StartP,
                                                       Curve.Segments[SegmentIndex].ControlP,
                                                       Curve.Segments[SegmentIndex].EndP,
                                                       Unlerp(Percent, TargetPercent, NextPercent));

            Result = TransformFunc(Poly, TargetPercent, Direction, Position);
                                   
            break;
        }

        Percent = NextPercent;
    }
    return Result;
}

