#include <stdarg.h>
#include "chr.h"
#include "chr_gamemath.h"
#include "dracogen.h"
#include "color.h"
#include "random.h"
#include "geom.h"
#include "bezier.h"

#if 1
global_variable f32 GlobalPixelsPerUnit = 4.0f;
global_variable f32 GlobalRenderScale = 5.0f;
#else
global_variable f32 GlobalPixelsPerUnit = 0.5f;
global_variable f32 GlobalRenderScale = 1.0f;
#endif

internal void
ClearBitmap(bitmap* Image, pixel_color Color = HEX_RGB(0x000000))
{
    for (int Y = 0; Y < Image->Height; ++Y)
    {
        uint8* DestRow = (uint8*)Image->Memory + (Image->BytesPerPixel * Image->Width) * Y;
        for (int X = 0; X < Image->Width; ++X)
        {
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * Image->BytesPerPixel);

            *DestPixel = Color;
        }
    }
}

internal void
ClearBuffer(game_offscreen_buffer* Image, pixel_color Color = HEX_RGB(0x000000))
{
    for (int Y = 0; Y < Image->Height; ++Y)
    {
        uint8* DestRow = (uint8*)Image->Memory + (Image->Pitch) * Y;
        for (int X = 0; X < Image->Width; ++X)
        {
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * 4);

            *DestPixel = Color;
        }
    }
}

internal void 
DrawBitmap(game_offscreen_buffer* Buffer, bitmap Image, int OffsetX = 0, int OffsetY = 0, int Scale = 3)
{
    int DestMinX = Max(OffsetX, 0);
    int DestMinY = Max(OffsetY, 0);
    int DestMaxX = Min(OffsetX + Scale*Image.Width, Buffer->Width);
    int DestMaxY = Min(OffsetY + Scale*Image.Height, Buffer->Height);
    
    int DestBytesPerPixel = Buffer->Pitch/Buffer->Width;

    for (int Y = DestMinY; Y < DestMaxY; ++Y)
    {
        uint8* SourceRow = (uint8*)Image.Memory + (Image.Width * Image.BytesPerPixel) * ((Y - DestMinY)/Scale);
        uint8* DestRow = (uint8*)Buffer->Memory + (Buffer->Pitch) * Y;
        for (int X = DestMinX; X < DestMaxX; ++X)
        {
            pixel_color* SourcePixel = (pixel_color*)(SourceRow + (X - DestMinX)/Scale * Image.BytesPerPixel);
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * DestBytesPerPixel);

            rgba_color SourceColor = FromU8Color(*SourcePixel);
            rgba_color DestColor = FromU8Color(*DestPixel);

            SourceColor.rgb *= SourceColor.a;
            DestColor = (1.0f - SourceColor.a) * DestColor + SourceColor;

            pixel_color ResultPixel = TO_U8_COLOR(DestColor);

            DestPixel->to.R = ResultPixel.R;
            DestPixel->to.G = ResultPixel.G;
            DestPixel->to.B = ResultPixel.B;
            DestPixel->to.A = ResultPixel.A;
        }
    }
}

internal void
DrawOvalSlice(bitmap* Buffer, vec2 Center, vec2 Radius, rgba_color Color, vec2 SliceStartDir, vec2 SliceEndDir, bool Blend = true)
{
    Color.rgb *= Color.a;
    pixel_color SrcColor = TO_U8_COLOR(Color);

    Center = (Center * (1.0f / GlobalPixelsPerUnit)) + (vec2)Buffer->Dim / 2.0f;
    Radius = Radius * (1.0f / GlobalPixelsPerUnit);
    int MinX = (int)Max(Center.X - Radius.X, 0);
    int MinY = (int)Max(Center.Y - Radius.Y, 0);
    int MaxX = (int)Min(Center.X + Radius.X, Buffer->Width);
    int MaxY = (int)Min(Center.Y + Radius.Y, Buffer->Height);

    for (int Y = MinY; Y < MaxY; ++Y) 
    {
        uint8* DestRow = (uint8*)Buffer->Memory + (Buffer->Width * Buffer->BytesPerPixel) * Y;
        for (int X = MinX; X < MaxX; ++X)
        {
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * Buffer->BytesPerPixel);
            vec2 Dir = V2(X, Y) - Center;
            vec3 A = V3(SliceStartDir, 0),
                 B = V3(Dir, 0),
                 C = V3(SliceEndDir, 0);

            if (Square(((f32)X - Center.X) / Radius.X) + Square(((f32)Y - Center.Y) / Radius.Y) < 1
                && (Cross(A, B).Z * Cross(A, C).Z >= 0 && Cross(C, B).Z * Cross(C, A).Z >= 0))
            {
                // Fill the pixel
                if (Blend) {
                    *DestPixel = (*DestPixel*(1.0f - Color.a) + SrcColor);
                } else {
                    *DestPixel = SrcColor;
                }
            }
        }
    }
}

internal void
DrawOval(bitmap* Buffer, vec2 Center, vec2 Radius, rgba_color Color, bool Blend = true) 
{
    return DrawOvalSlice(Buffer, Center, Radius, Color, V2(1, 0), V2(1, 0), Blend);
}


internal void
DrawTriangle(bitmap* Buffer, tri Tri, rgba_color Color, bool Blend = true)
{
    Color.rgb *= Color.a;
    pixel_color SrcColor = TO_U8_COLOR(Color);
    Tri = (Tri * (1.0f / GlobalPixelsPerUnit)) + (vec2)Buffer->Dim / 2.0f;

    int MinX = (int)Max(Min(Min(Tri.Verts[0].X, Tri.Verts[1].X), Tri.Verts[2].X), 0);
    int MaxX = 1 + (int)Min(Max(Max(Tri.Verts[0].X, Tri.Verts[1].X), Tri.Verts[2].X), Buffer->Width);
    int MinY = (int)Max(Min(Min(Tri.Verts[0].Y, Tri.Verts[1].Y), Tri.Verts[2].Y), 0);
    int MaxY = 1 + (int)Min(Max(Max(Tri.Verts[0].Y, Tri.Verts[1].Y), Tri.Verts[2].Y), Buffer->Width);

    for (int Y = MinY; Y < MaxY; ++Y) 
    {
        uint8* DestRow = (uint8*)Buffer->Memory + (Buffer->Width * Buffer->BytesPerPixel) * Y;
        for (int X = MinX; X < MaxX; ++X)
        {
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * Buffer->BytesPerPixel);

            vec2 v0 = Tri.Verts[2] - Tri.Verts[0];
            vec2 v1 = Tri.Verts[1] - Tri.Verts[0];
            vec2 v2 = V2(X, Y) - Tri.Verts[0];

            f32 dot00 = Inner(v0, v0);
            f32 dot01 = Inner(v0, v1);
            f32 dot02 = Inner(v0, v2);
            f32 dot11 = Inner(v1, v1);
            f32 dot12 = Inner(v1, v2);

            f32 invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
            f32 u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            f32 v = (dot00 * dot12 - dot01 * dot02) * invDenom;

            if (u >= 0.0f && v >= 0.0f && (u + v <= 1.0f)) {
                if (Blend) {
                    *DestPixel = (*DestPixel*(1.0f - Color.a) + SrcColor);
                } else {
                    *DestPixel = SrcColor;
                }
            }
        }
    }
}

internal void
DrawQuad(bitmap* Buffer, quad Quad, rgba_color Color, bool Blend = true)
{
    tri Tri1 = { Quad.Verts[0], Quad.Verts[1], Quad.Verts[2] };
    tri Tri2 = { Quad.Verts[2], Quad.Verts[3], Quad.Verts[0] };
    DrawTriangle(Buffer, Tri1, Color, Blend);
    DrawTriangle(Buffer, Tri2, Color, Blend);
}

template<int N>
internal void
DrawPoly(bitmap* Buffer, verts<N> Poly, rgba_color Color, bool Blend = true)
{
    for (int VertIndex = 2; VertIndex < N; ++VertIndex)
    {
        // Convex polyhedron
        tri Tri = { Poly.Verts[0], Poly.Verts[VertIndex - 1], Poly.Verts[VertIndex] };
        DrawTriangle(Buffer, Tri, Color, Blend);
    }
}

internal void
DrawLine(bitmap* Buffer, int NumVertices, vec2* Vertices, 
         f32 (*Thickness)(f32 Percent, void* Param), rgba_color Color, bool MiterEnds=true, void* Param = NULL, 
         bool Blend = true)
{
    Assert(NumVertices > 1);
    f32 TotalLength = 0.0f;
    for (int i = 1; i < NumVertices; ++i) {
        TotalLength += Length(Vertices[i - 1] - Vertices[i]);
    }

    f32 LenPercent = 0.0f;
    for (int i = 1; i < NumVertices; ++i) {
        vec2 Start = Vertices[i - 1];
        vec2 End = Vertices[i];

        vec2 Segment = End - Start;
        vec2 Direction = Normalize(Segment);
        vec2 LeftNormal = V2(Direction.Y, -Direction.X);
        vec2 RightNormal = V2(-Direction.Y, Direction.X);

        f32 dLenPercent = Length(Segment) / TotalLength;
        f32 StartThickness = Thickness(LenPercent, Param);
        f32 EndThickness = Thickness(LenPercent + dLenPercent, Param);

        quad SegmentQuad = {};
        SegmentQuad.Verts[0] = Start + LeftNormal*StartThickness*0.5f;
        SegmentQuad.Verts[1] = Start + RightNormal*StartThickness*0.5f;
        SegmentQuad.Verts[2] = End + RightNormal*EndThickness*0.5f;
        SegmentQuad.Verts[3] = End + LeftNormal*EndThickness*0.5f;

        DrawQuad(Buffer, SegmentQuad, Color, Blend);

        if (MiterEnds && i == 1) 
        {
            DrawOvalSlice(Buffer, Start, V2(StartThickness/2,StartThickness/2), Color,
                          LeftNormal, RightNormal, Blend);
        }

        if (i < NumVertices - 1)
        {
            vec2 NextSegment = Vertices[i + 1] - Vertices[i];

            vec2 NextLeftOrtho = V2(NextSegment.Y, -NextSegment.X);
            vec2 NextRightOrtho = V2(-NextSegment.Y, NextSegment.X);

            if (Inner(RightNormal, NextRightOrtho) != Inner(RightNormal,RightNormal)) {
                DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color, 
                              RightNormal, NextRightOrtho, Blend);
            }

            if (Inner(LeftNormal, NextLeftOrtho) != Inner(RightNormal,RightNormal)) {
                DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color, 
                              LeftNormal, NextLeftOrtho, Blend);
            }
        }

        if (MiterEnds)
        {
            DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color,
                          LeftNormal, RightNormal, Blend);
        }

        LenPercent += dLenPercent;
    }
}

internal f32 
ThicknessFuncWrapper(f32 Percent, void* Param)
{
    typedef f32 (*thickness_func)(f32 Percent);
    thickness_func Func = (thickness_func)Param;
    return Func(Percent);
}

internal void
DrawLine(bitmap* Buffer, int NumVertices, vec2* Vertices, 
         f32 (*Thickness)(f32 Percent), rgba_color Color, bool MiterEnds=true, 
         bool Blend = true)
{
    return DrawLine(Buffer, NumVertices, Vertices, ThicknessFuncWrapper, Color, MiterEnds, (void*)Thickness, Blend);
}


struct bezier_thickness_params
{
    range PercentRange;
    f32 (*ThicknessFunc)(f32 Percent);
};

internal f32
BezierThicknessFunc(f32 Percent, void* Param)
{
    bezier_thickness_params* Params = (bezier_thickness_params*)Param;
    Percent = Lerp(Params->PercentRange.Min, Percent, Params->PercentRange.Max);
    return Params->ThicknessFunc(Percent);
}

internal void
DrawBezierCurveSegment(bitmap* Target, int CurveCount, verts<3>* Curves, f32 TotalLength,
                       f32 (*Thickness)(f32 Percent), rgba_color Color, 
                       int SegmentsPerCurve = 10, bool MiterEnds = true)
{
    //TODO(chronister): Is there a good way to approximate this length more quickly?
    f32 Percent = 0.0f;
    for (int CurveIndex = 0; CurveIndex < CurveCount; ++CurveIndex)
    {
        verts<3> CurveSegment = Curves[CurveIndex];
        for (int SegmentIndex = 0; SegmentIndex < SegmentsPerCurve; ++SegmentIndex)
        {
            f32 t = (f32)SegmentIndex / (f32)SegmentsPerCurve;
            f32 tNext = (f32)(SegmentIndex + 1) / (f32)SegmentsPerCurve;
            
            vec2 Line[2];
            Line[0] = BezierQuadratic(CurveSegment.Verts[0],
                                      CurveSegment.Verts[1],
                                      CurveSegment.Verts[2], t);
            Line[1] = BezierQuadratic(CurveSegment.Verts[0],
                                      CurveSegment.Verts[1],
                                      CurveSegment.Verts[2], tNext);
            f32 SegmentLength = Length(Line[1] - Line[0]);
            f32 NextPercent = Percent + (SegmentLength / TotalLength);

            bezier_thickness_params Params = { Lerp(Percent, t, NextPercent), 
                                               Lerp(Percent, tNext, NextPercent), 
                                               Thickness };
            DrawLine(Target, 2, Line, BezierThicknessFunc, Color, 
                     MiterEnds || (0 < SegmentIndex && SegmentIndex < SegmentsPerCurve - 1) || (0 < CurveIndex && CurveIndex < CurveCount - 1), 
                     (void*)&Params);

            Percent = NextPercent;
        }
    }
}

static f32 SnootThicknessBase = 210.0f;
static f32 SnootThicknessScale = -3.0f;
static f32 SnootPartsProportion = 0.8f;
static f32 SnootMouthSize = 0.2f;
static f32 SnootMouthOverbite = 0.0f;

internal f32
SnootThickness(f32 Percent)
{
    return SnootThicknessScale * Pow(Percent, 0.05f) + SnootThicknessBase;
}

internal f32
SnootTopThickness(f32 Percent)
{
    return SnootPartsProportion * SnootThickness(Percent);
}

internal f32
SnootTopHighlightThickness(f32 Percent)
{
    return Pow(0.95f, Square(Percent)) * SnootTopThickness(Percent);
}

internal f32
SnootBottomThickness(f32 Percent)
{
    return (1.0f - SnootPartsProportion) * SnootThickness(Percent);
}

internal f32
SnootBottomHighlightThickness(f32 Percent)
{
    return 0.8f * SnootBottomThickness(Percent);
}

internal vec2
SnootTopOffset(f32 Percent)
{
    return V2((1.0f - Percent) * -10.0f, (SnootThickness(Percent) - SnootTopThickness(Percent)) / 2 + 
              SnootMouthSize * SnootThicknessBase * Percent);
}

internal vec2
SnootTopHighlightOffset(f32 Percent)
{
    return SnootTopOffset(Percent) - V2(0, SnootTopThickness(Percent) - SnootTopHighlightThickness(Percent));
}

internal vec2
SnootBottomOffset(f32 Percent)
{
    return V2(SnootMouthOverbite * Percent, -(SnootThickness(Percent) - SnootBottomThickness(Percent)) / 2 +
              -SnootMouthSize * SnootThicknessBase * Percent);
}

internal vec2
SnootBottomHighlightOffset(f32 Percent)
{
    return SnootBottomOffset(Percent) + V2(-10*(1.0f - Percent), SnootBottomThickness(Percent) - SnootBottomHighlightThickness(Percent));
}

internal f32
MouthThickness(f32 Percent)
{
    return (1.0f - Percent) * SnootBottomThickness(Percent);
}

internal f32
MouthLineThickness(f32 Percent)
{
    return 5.0f - 4.0f*Percent;
}

internal vec2
MouthOffset(f32 Percent)
{
    return V2(SnootMouthOverbite * Percent, 
              SnootBottomOffset(Percent).Y + SnootBottomThickness(Percent) * 0.5f + MouthThickness(Percent) * 0.5f);
}

internal vec2
MouthLineTopOffset(f32 Percent)
{
    return SnootTopOffset(Percent) - 0.5f*V2(0, SnootTopThickness(Percent));
}

internal vec2
MouthLineBottomOffset(f32 Percent)
{
    return SnootBottomOffset(Percent) + 0.5f*V2(0, SnootBottomThickness(Percent));
}


internal f32
MouthClearThickness(f32 Percent)
{
    return 0.7f*SnootThickness(Percent);
}

internal vec2
MouthClearOffset(f32 Percent)
{
    return V2((1.0f - Percent) * 10.0f, 0);
}

internal vec2
SnootFrontToothOffset(f32 Percent)
{
    return SnootBottomOffset(Percent) + 0.5f*V2((0.5f - Percent) * 40.0f, SnootBottomThickness(Percent));
}

internal vec2
SnootBackToothOffset(f32 Percent)
{
    return MouthOffset(Percent) + 
        0.5f*V2((0.5f - Percent) * 20.0f, MouthThickness(Percent));
}

internal verts<1>
SnootNoseTransformFunc(verts<1> Nostril, f32 Percent, vec2 Direction, vec2 Position)
{
    return Nostril + Position + SnootTopOffset(Percent) + 
             Normalize(V2(-Direction.Y, Direction.X)) * SnootTopThickness(Percent)*0.4f;
}

static f32 NeckThicknessBase = 210.0f;
static f32 NeckThicknessScale = 30.0f;

internal f32
NeckThickness(f32 Percent)
{
    return NeckThicknessScale*Percent + NeckThicknessBase;
}

internal f32
NeckHighlightThickness(f32 Percent)
{
    return 0.7f * NeckThickness(Percent);
}

static vec2 LastNeckPlatePos = V2(0,0);
static vec2 LastNeckPlateHighlightPos = V2(0,0);
static vec2 LastSecondaryNeckPlatePos = V2(0,0);
static vec2 LastNeckSpinePos = V2(0,0);
static f32 MinNeckPlateDist = 35.0f;
static f32 MinNeckSpineDist = 30.0f;
static vec2 HeadPosRef = V2(0,0);
static vec2 NeckSpineDim = V2(5, 7);
static f32 NeckSecondaryPlateRadius = 0.22f;

internal verts<5>
NeckPlateTransformFunc(verts<5> Plate, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position + NeckThickness(Percent) * (0.42f - 0.15f*Percent) * LeftNormal(Direction);
    if ((Percent < 0.01f || Length(TranslatePos - LastNeckPlatePos) > MinNeckPlateDist)
        && (Length(HeadPosRef - TranslatePos) > 80.0f))
    {
        LastNeckPlatePos = TranslatePos;
        
        Plate = Plate * (15.0f + 20.0f * Percent);
        Plate = RotateAroundOrigin(Plate, ATan2(Direction.Y, Direction.X));
        Plate = Plate + TranslatePos;
        return Plate;
    }

    //TODO(chronister): HACK!
    return Plate + V2(-1000,-1000);
}

internal verts<5>
NeckPlateHighlightTransformFunc(verts<5> Plate, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position + NeckThickness(Percent) * (0.42f - 0.15f*Percent) * LeftNormal(Direction);
    if ((Percent < 0.01f || Length(TranslatePos - LastNeckPlateHighlightPos) > MinNeckPlateDist)
        && (Length(HeadPosRef - TranslatePos) > 80.0f))
    {
        LastNeckPlateHighlightPos = TranslatePos;
        
        Plate = Plate * (15.0f + 20.0f * Percent);
        Plate = RotateAroundOrigin(Plate, ATan2(Direction.Y, Direction.X));
        Plate = Plate + TranslatePos;
        return Plate;
    }

    //TODO(chronister): HACK!
    return Plate + V2(-1000,-1000);
}

internal verts<5>
NeckSecondaryPlateTransformFunc(verts<5> Plate, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position + NeckThickness(Percent) * NeckSecondaryPlateRadius * LeftNormal(Direction);
    if ((Percent < 0.01f || Length(TranslatePos - LastSecondaryNeckPlatePos) > MinNeckPlateDist)
        && (Length(HeadPosRef - TranslatePos) > 80.0f))
    {
        LastSecondaryNeckPlatePos = TranslatePos;
        
        Plate = Plate * (5.0f + 10.0f * Percent);
        Plate = RotateAroundOrigin(Plate, ATan2(Direction.Y, Direction.X));
        Plate = Plate + TranslatePos;
        return Plate;
    }

    //TODO(chronister): HACK!
    return Plate + V2(-1000,-1000);
}

internal verts<5>
NeckSpineTransformFunc(verts<5> Spine, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position - NeckThickness(Percent) * 0.47f * Normalize(V2(-Direction.Y, Direction.X));
    if ((Percent < 0.01f || Length(TranslatePos - LastNeckSpinePos) > MinNeckSpineDist))
    {
        LastNeckSpinePos = TranslatePos;

        Spine = Spine * NeckSpineDim;
        Spine = RotateAroundOrigin(Spine, PI32 + ATan2(Direction.Y, Direction.X));
        Spine = Spine + TranslatePos;
        return Spine;
    }

    //TODO(chronister): HACK!
    return Spine + V2(-1000, -1000);
}

static f32 NeckGlintRadius = 0.22f;

internal verts<1>
ScaleGlintTransformFunc(verts<1> Glint, f32 Percent, vec2 Direction, vec2 Position)
{
    return Glint + Position + RightNormal(Direction) * NeckGlintRadius*NeckThickness(Percent);
}

static f32 HornThicknessBase = 50.0f;
static f32 HornThicknessScale = -45.0f;

internal f32
HornThickness(f32 Percent)
{
    return HornThicknessScale*Percent + HornThicknessBase;
}

internal f32
TestThickness(f32 Percent)
{
    return -18.0f*Percent + 20.0f;
}

internal void
DrawDragon(bitmap* Target, rgba_color BGColor, f32 t)
{
    hsva_color ScaleColor = RandHSV(BETWEEN(0.0f, 360.0f), BETWEEN(0.3f, 0.7f), BETWEEN(0.3f, 0.6f));
    hsva_color EyeColor = RandHSV(AROUND(ScaleColor.H, 90), BETWEEN(0.7f, 1.0f), BETWEEN(0.6f, 0.8f));
    hsva_color FinColor = RandHSV(BETWEEN(0.0f, 360.0f), BETWEEN(0.4f, 0.8f), BETWEEN(0.4f, 0.6f));
    hsva_color MouthColor = RandHSV(AROUND(ScaleColor.H, 5), BETWEEN(0.4f, 0.8f), BETWEEN(0.2f, 0.3f));

    hsva_color HornColor = {};
    HornColor.H = ScaleColor.H + RandSign() * RandRange(45, 90);
    HornColor.S = RandRange(0.4f, 0.7f);
    HornColor.V = RandRange(0.4f, 0.7f);
    HornColor.A = 1.0f;
    hsva_color ToothColor = RandHSV(AROUND(HornColor.H, 5), BETWEEN(0.0f, 0.3f), BETWEEN(0.8f, 1.0f));

    hsva_color PlateColor = HornColor;
    PlateColor.S = ScaleColor.S;
    PlateColor.V = ScaleColor.V;

    Assert(HornColor.A == 1.0f);
    Assert(PlateColor.A == 1.0f);

    vec2 HeadDim = V2(RandRange(70, 85), RandRange(60, 75));
    vec2 HeadOffset = HeadPosRef = V2(RandRange(-20, 40), RandRange(-10, 20));

    SnootThicknessBase = HeadDim.Y * RandRange(1.5f, 1.8f);
    SnootThicknessScale = RandRange(-10, -35);
    SnootMouthSize = RandRange(0.0f, 0.3f);
    SnootMouthOverbite = RandRange(-20.0f, 2.0f);

    NeckThicknessBase = HeadDim.Y*2;
    NeckThicknessScale = RandRange(20, 50);
    NeckSpineDim = V2(RandRange(4, 7), RandRange(5, 8));

    f32 HornThicknessBaseLocal = RandRange(30, 60);
    f32 HornThicknessScaleLocal = -HornThicknessBaseLocal + RandRange(5, 8);

    int NumHorns = RandChoice(3) + 1;

    vec2 EyeSize = V2(RandRange(20, 30), RandRange(10, 15));
    vec2 EyeOffset = V2(RandRange(10, 20), 15);

    vec2 FarEyeOffset = V2(HeadDim.X - 20, Min(SnootThicknessBase - HeadDim.Y - 1.5*EyeSize.Y, HeadDim.Y * 0.6f));

    vec2 HornOffset = V2(RandRange(-30, -50), RandRange(10, 20));
    vec2 FarHornOffset = V2(1*HeadDim.X/3, 0.5f*FarEyeOffset.Y);
    f32 HornSpacingAngle = RandRange(0.7f, 1.4f);
    f32 HornAngleStart = PI32 - RandRange(0.7f, 1.4f);
    f32 HornRadiusScale = RandRange(0.8f, 0.9f);

    f32 HeadAngle = RandRange(-PI32/4.0f, PI32/4.0f);
    vec2 HeadDir = NeckThicknessScale*V2(Cos(HeadAngle), Sin(HeadAngle));

    bezier_curve<3> Neck = CurveByOffsets<3>(
          HeadOffset, 
              -HeadDir,
              V2(RandRange(AROUND(-70, 20)), RandRange(-50, -10)),

              V2(RandRange(-110, -70), RandRange(AROUND(-70, 20))), V2(RandRange(AROUND(90, 10)), RandRange(AROUND(-100, 10))),

              V2(170, -100), V2(-200, -100));

    verts<4> Snoot = VerticesByOffsets<4>(V2(5,-HeadDim.Y + SnootThicknessBase/2),
                                          V2(40,RandRange(0, -2)),
                                          V2(RandRange(25, 35),RandRange(0, -5)),
                                          V2(RandRange(30, 50),RandRange(0, -5))) + HeadOffset;


    verts<5> Horn = VerticesByOffsets<5>(V2(0, 0),
                                         V2(RandRange(-30, -60), RandRange(20, 40)),
                                         V2(RandRange(5, -90), RandRange(5, 50)),
                                         V2(-10, 10),
                                         V2(RandRange(-5, -35), RandRange(5, 20)));

    Assert(HornColor.A == 1.0f);
    Assert(PlateColor.A == 1.0f);

#if 1
    for (int HornIndex = NumHorns - 1; HornIndex >= 0; --HornIndex) 
    {
        f32 Angle = HornAngleStart + HornSpacingAngle*HornIndex;

        DrawLine(Target, ArrayCount(Horn.Verts), 
                 (RotateAroundOrigin(Horn * Pow(0.8f, HornIndex), 
                                     HornIndex * HornSpacingAngle * 0.5f) + 
                  Hadamard(HeadDim*HornRadiusScale, V2(0.5f*Cos(Angle), 0.5f*Sin(Angle))) + 
                  V2(-20, -40) * Pow(HornIndex / 3.0f, 2.0f) + 
                  HornOffset + FarHornOffset + HeadOffset).Verts, 
                 HornThickness, HSVtoRGB(DeltaV(HornColor, -0.2f - 0.03f*HornIndex)), true);

        HornThicknessBase = HornThicknessBaseLocal * Pow(0.8f, HornIndex);
        HornThicknessScale = HornThicknessScaleLocal * Pow(0.8f, HornIndex);
    }

    Assert(HornColor.A == 1.0f);
    Assert(PlateColor.A == 1.0f);

#if 0
    DrawOval(Target, HeadOffset + EyeOffset + FarEyeOffset, EyeSize + V2(-7.5f, -1), TO_U8_COLOR(EyeColor));
    DrawOval(Target, HeadOffset + EyeOffset + FarEyeOffset, V2(4, EyeSize.Y), TO_U8_COLOR(EyeColor) * 0.2f);
#endif

    int NumSpines = 20;
    for (int i = 0; i <= NumSpines; ++i)
    {
        verts<5> Spine = Vertices<5>(V2(-3, 0), V2(4, 0), V2(2, 2), V2(-4, 3), V2(-3, 2));
        DrawPoly(Target, 
                 CurveTransformParametric(Neck, Spine, ((f32)i / (f32)NumSpines), NeckSpineTransformFunc),
                 HSVtoRGB(HornColor));

#if 0
        verts<1> Pt = Vertices<1>(V2(0,0));
        DrawOval(Target,
                 CurveTransformParametric(Neck, Pt, ((f32)i / (f32)NumSpines), NeckPlateTransformFunc).Verts[0],
                 V2(5,5), HEX_RGB((int)(((f32)i / (f32)NumSpines) * 255) << 16));
#endif
    }

    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           NeckThickness, HSVtoRGB(ScaleColor), 9, true);

    // Neck highlight
    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           NeckHighlightThickness, HSVtoRGB(DeltaV(ScaleColor, 0.03f)), 9, true);

    DrawOval(Target, HeadOffset, HeadDim, HSVtoRGB(ScaleColor));

    DrawOval(Target, HeadOffset - V2(HeadDim.X * 0.1f, 0), HeadDim * 0.9f, HSVtoRGB(DeltaV(ScaleColor, 0.03f)));

    for (int i = 0; i < 8; ++i)
    {
        NeckGlintRadius = 0.22f;
        DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.10f + i*0.05f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(15, 10),
                 Hadamard(V4(1,1,1,0.2f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));

        NeckGlintRadius = 0.10f;
        DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.07f + i*0.05f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(15, 10),
                 Hadamard(V4(1,1,1,0.2f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));

        NeckGlintRadius = -0.15f;
        DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.25f + i*0.1f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(15, 10),
                 Hadamard(V4(1,1,1,0.2f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));

        if (i > 0) {
            NeckGlintRadius = 0.15f;
            DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.12f + i*0.05f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(10, 7),
                 Hadamard(V4(1,1,1,0.4f), HSVtoRGB(DeltaHSV(ScaleColor, 0, -0.2f, 0.3f))));

            NeckGlintRadius = -0.03f;
            DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.07f + i*0.05f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(10, 7),
                 Hadamard(V4(1,1,1,0.15f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));

            NeckGlintRadius = 0.34f;
            DrawOval(Target, 
                 CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.12f + i*0.05f, 
                                          ScaleGlintTransformFunc).Verts[0],
                 V2(10, 7),
                 Hadamard(V4(1,1,1,0.15f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));

            NeckGlintRadius = -0.22f;
            DrawOval(Target, 
                     CurveTransformParametric(Neck, Vertices<1>(V2(0,0)), 0.29f + i*0.1f, 
                                              ScaleGlintTransformFunc).Verts[0],
                     V2(10, 7),
                     Hadamard(V4(1,1,1,0.2f), HSVtoRGB(DeltaV(ScaleColor, 0.3f))));
        }
    }

    int PlateLayers = RandChoice(3) + 1;
    if (RandChoice(2) == 1) {
        int NumPlates = RandRange(20, 22);
        f32 PlateConcavityFactor = RandRange(0.0f, 0.4f);
        verts<5> Plate = Vertices<5>(V2(PlateConcavityFactor, 0.0f), V2(0, -1), V2(-1.0f, -0.9f), V2(-1.0f, 0.9f), V2(0, 1));
        for (int i = 0; i <= NumPlates; ++i)
        {
            DrawPoly(Target, 
                     CurveTransformParametric(Neck, Plate, 0.16f + 0.92f*((f32)i / (f32)NumPlates), 
                                              NeckPlateTransformFunc),
                     HSVtoRGB(PlateColor));

            DrawPoly(Target, 
                     CurveTransformParametric(Neck, Plate*0.9f + V2(-0.2f, 0.2f), 0.16f + 0.92f*((f32)i / (f32)NumPlates), 
                                              NeckPlateHighlightTransformFunc),
                     HSVtoRGB(DeltaV(PlateColor, 0.05f)));

            if (i < NumPlates && PlateLayers >= 2) {
                NeckSecondaryPlateRadius = 0.22f;
                DrawPoly(Target, 
                         CurveTransformParametric(Neck, Plate, 0.08f + 0.92f*((f32)(i + 0.5f) / (f32)NumPlates), 
                                                  NeckSecondaryPlateTransformFunc),
                         HSVtoRGB(PlateColor));

                if (i < NumPlates && PlateLayers >= 3) {
                    NeckSecondaryPlateRadius = 0.12f;
                    DrawPoly(Target, 
                             CurveTransformParametric(Neck, Plate*0.8f, 0.10f + 0.92f*((f32)(i + 0.5f) / (f32)NumPlates), 
                                                      NeckSecondaryPlateTransformFunc),
                             HSVtoRGB(PlateColor));
                }
            }
        }
    }

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthClearOffset).Verts, 
             MouthClearThickness, BGColor, false, false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthOffset).Verts, 
             MouthThickness, HSVtoRGB(MouthColor), false);

    for (int i = 0; i < ArrayCount(Snoot.Verts); ++i)
    {
        tri Tooth = Vertices<3>(V2(-1, 0), V2(0, (f32)SQRT3), V2(1, 0));
        DrawTriangle(Target, 
                     Tooth * 4 + 
                     LineOffsetParametric(Snoot, SnootBackToothOffset).Verts[i], 
                     HSVtoRGB(DeltaV(ToothColor, -0.3f)));

        DrawTriangle(Target, 
                     Tooth * 6 + 
                     LineOffsetParametric(Snoot, SnootFrontToothOffset).Verts[i], 
                     HSVtoRGB(ToothColor));
    }

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootBottomOffset).Verts, 
             SnootBottomThickness, HSVtoRGB(ScaleColor), false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootBottomHighlightOffset).Verts, 
             SnootBottomHighlightThickness, HSVtoRGB(DeltaV(ScaleColor, 0.03f)), false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootTopOffset).Verts, 
             SnootTopThickness, HSVtoRGB(ScaleColor), false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootTopOffset).Verts, 
             SnootTopThickness, HSVtoRGB(DeltaV(ScaleColor, 0.03f)), false);

#if 0
    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthLineTopOffset).Verts, 
             MouthLineThickness, HSVtoRGB(DeltaV(ScaleColor, -0.03f)), false);
#else
    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthLineBottomOffset).Verts, 
             MouthLineThickness, HSVtoRGB(DeltaV(ScaleColor, -0.03f)), false);
#endif


    for (int HornIndex = NumHorns - 1; HornIndex >= 0; --HornIndex) 
    {
        f32 Angle = HornAngleStart + HornSpacingAngle*HornIndex;
        vec2 HornPosition = Hadamard(HeadDim*HornRadiusScale, V2(0.5f*Cos(Angle), 0.5f*Sin(Angle))) +
                                V2(-20, -40) * Pow(HornIndex / 3.0f, 2.0f) + 
                                HornOffset + HeadOffset;

        DrawOval(Target, HornPosition-V2(4,0), Hadamard(V2(0.60f,0.62f),V2(HornThicknessBase,HornThicknessBase)),
                 FromU8Color(HEX_RGB(0x05000000)));

        DrawOval(Target, HornPosition-V2(4,5), Hadamard(V2(0.55f,0.55f),V2(HornThicknessBase,HornThicknessBase)),
                 FromU8Color(HEX_RGB(0x22000000)));

        HornThicknessBase = HornThicknessBaseLocal * Pow(0.8f, HornIndex);
    }

    for (int HornIndex = NumHorns - 1; HornIndex >= 0; --HornIndex) 
    {
        f32 Angle = HornAngleStart + HornSpacingAngle*HornIndex;
        vec2 HornPosition = Hadamard(HeadDim*HornRadiusScale, V2(0.5f*Cos(Angle), 0.5f*Sin(Angle))) +
                                V2(-20, -40) * Pow(HornIndex / 3.0f, 2.0f) + 
                                HornOffset + HeadOffset;

        DrawLine(Target, ArrayCount(Horn.Verts), 
                 (RotateAroundOrigin(Horn * Pow(0.8f, HornIndex), 
                                     HornIndex * HornSpacingAngle * 0.5f) + 
                  HornPosition).Verts,
                 HornThickness, HSVtoRGB(DeltaV(HornColor, -0.04f)), true);

        HornThicknessBase *= 0.8f;
        HornThicknessScale *= 0.8f;

        DrawLine(Target, ArrayCount(Horn.Verts),
                 (RotateAroundOrigin(Horn * Pow(0.8f, HornIndex), 
                                     HornIndex * HornSpacingAngle * 0.5f) + 
                  HornPosition).Verts, 
                 HornThickness, HSVtoRGB(HornColor), true);

        HornThicknessBase = HornThicknessBaseLocal * Pow(0.8f, HornIndex);
        HornThicknessScale = HornThicknessScaleLocal * Pow(0.8f, HornIndex);
    }

    DrawOval(Target, HeadOffset + EyeOffset + V2(RandRange(-3,3),RandSign()*RandRange(2,5)), 
             1.1f*EyeSize, FromU8Color(HEX_RGB(0x33000000)));
    DrawOval(Target, HeadOffset + EyeOffset, EyeSize, HSVtoRGB(EyeColor));
    DrawOval(Target, HeadOffset + EyeOffset, 0.8f*EyeSize, HSVtoRGB(DeltaHSV(EyeColor, 0, -0.1f,0.1f)));
    f32 PupilX = RandRange(-0.4f, 0.7f)*EyeSize.X;
    f32 PupilHeight = 1.0f*EyeSize.Y * SquareRoot(1 - Square(PupilX / EyeSize.X));
    DrawOval(Target, HeadOffset + EyeOffset + V2(PupilX, 0), V2(RandRange(5, 7), PupilHeight), HSVtoRGB(DeltaV(EyeColor, -0.8f)));

    hsva_color GlintColor = (DeltaHSV(EyeColor, 0, -0.8f,0.9f));
    GlintColor.A = 0.2f;
    DrawOval(Target, HeadOffset + EyeOffset + V2(0, 3), V2(0.6f*EyeSize.X, 0.3f*EyeSize.Y), HSVtoRGB(GlintColor));

    DrawOval(Target, 
                 LineTransformParametric(Snoot, Vertices<1>(V2(0,0)), 0.8f, SnootNoseTransformFunc).Verts[0], 
                 V2(10,5),
                 HSVtoRGB(DeltaV(ScaleColor, -0.8f)));

#endif

#if 0
    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           TestThickness, FromU8Color(HEX_RGB(0xffff0000)), 9, true);

    for (int i = 0; i < ArrayCount(Neck.Segments); ++i) 
    {
        DrawOval(Target, Neck.Segments[i].StartP, V2(10,10), FromU8Color(HEX_RGB(0xffeeaa00)));
        DrawOval(Target, Neck.Segments[i].ControlP, V2(10,10), FromU8Color(HEX_RGB(0xffeeaa00)));
        DrawOval(Target, Neck.Segments[i].EndP, V2(10,10), FromU8Color(HEX_RGB(0xffeeaa00)));
    }


#endif

#if 0
    // TEST

    bezier_curve<1> Test = CurveByOffsets<1>(
              V2(30*Sin(t), 30),
              V2(-80*Cos(t), -30),
              V2(100*Cos(t), 100*Sin(t)));

    vec2 X0 = BezierQuadratic(Test.Segments[0].StartP,
                              Test.Segments[0].ControlP,
                              Test.Segments[0].EndP,
                              0.4675f);

    DrawBezierCurveSegment(Target, 1, (verts<3>*)&Test.Segments, Test.TotalLength,
                           TestThickness, FromU8Color(HEX_RGB(0xffffffff)), 9, true);

    DrawOval(Target, Test.Segments[0].StartP, V2(5,5), FromU8Color(HEX_RGB(0xff888888)));
    DrawOval(Target, Test.Segments[0].ControlP, V2(5,5), FromU8Color(HEX_RGB(0xff888888)));
    DrawOval(Target, Test.Segments[0].EndP, V2(5,5), FromU8Color(HEX_RGB(0xff888888)));

    DrawOval(Target, X0, V2(8,8), FromU8Color(HEX_RGB(0xffff0000)));

    vec4 tCurve = BezierQuadraticInverse(Test.Segments[0].StartP,
                                    Test.Segments[0].ControlP,
                                    Test.Segments[0].EndP,
                                    X0);

    u32 Colors[4] = { 0xffccaa22, 0xff22ccaa, 0xffcc22aa, 0xff5500cc };
    for (int i = 0; i < 4; ++i) 
    {
        vec2 X = BezierQuadratic(Test.Segments[0].StartP,
                                 Test.Segments[0].ControlP,
                                 Test.Segments[0].EndP,
                                 tCurve.E[i]);

        DrawOval(Target, X, V2(7-i,7-i), FromU8Color(HEX_RGB(Colors[i])));
    }

    // END TEST
#endif
}

enum background_type
{
    BT_Cave,
    BT_Field,
    BT_Mountain,
    BT_Count,
};

internal void
DrawBackground(bitmap* Target, rgba_color BGColor, f32 t)
{
    background_type Type = (background_type)RandChoice(BT_Count);
#if 1
    Type = BT_Cave;
#endif
    switch (Type) {
        case BT_Cave:
        {
            hsva_color CaveColor = RandHSV(BETWEEN(0, 360), BETWEEN(0.0f, 0.3f), BETWEEN(0.09f, 0.15f));

            quad BGQuad = Vertices<4>(V2(-256, -256), V2(-256, 256), V2(256, 256), V2(256, -256));
            DrawQuad(Target, BGQuad, HSVtoRGB(CaveColor));
            DrawOval(Target, V2(0,0), V2(RandRange(AROUND(240, 10)), RandRange(AROUND(230, 10))), 
                     HSVtoRGB(DeltaV(CaveColor, -0.03f)));
            DrawOval(Target, V2(0,0), V2(RandRange(AROUND(205, 10)), RandRange(AROUND(195, 10))), 
                     HSVtoRGB(DeltaV(CaveColor, -0.06f)));

            tri Stalactite = Vertices<3>(V2(-0.5f, 0), V2(0.0f, -7.0f), V2(0.5f, 0.0f));

            int NumStalactites = RandChoice(8) + 12;
            for (int i = 0; i <= NumStalactites; ++i) 
            {
                f32 X = (f32)i / (f32)NumStalactites;
                f32 PositionFactor = -4*Square(X) + 4*X;
                DrawTriangle(Target, Stalactite * (RandRange(16.0f, 24.0f) + (1.0f - PositionFactor) * 20.0f)
                             + V2((X - 0.5f) * 400.0f, 190.0f + PositionFactor * 40.0f), 
                             HSVtoRGB(DeltaV(CaveColor, -0.03f)));
            }

            NumStalactites = RandChoice(8) + 8;
            for (int i = 0; i < NumStalactites; ++i) 
            {
                f32 X = (f32)i / (f32)NumStalactites;
                f32 PositionFactor = -4*Square(X) + 4*X;
                DrawTriangle(Target, Stalactite * (RandRange(18.0f, 42.0f)  + (1.0f - PositionFactor) * 10.0f)
                             + V2((X - 0.5f) * 400.0f, 280.0f + PositionFactor * 60.0f), 
                             HSVtoRGB(CaveColor));
            }

            NumStalactites = RandChoice(8) + 12;
            for (int i = 0; i <= NumStalactites; ++i) 
            {
                f32 X = (f32)i / (f32)NumStalactites;
                f32 PositionFactor = -4*Square(X) + 4*X;
                DrawTriangle(Target, Stalactite * -(RandRange(16.0f, 24.0f) + (1.0f - PositionFactor) * 20.0f)
                             + V2((X - 0.5f) * 400.0f, -210.0f - PositionFactor * 40.0f), 
                             HSVtoRGB(DeltaV(CaveColor, -0.03f)));
            }

            NumStalactites = RandChoice(8) + 8;
            for (int i = 0; i < NumStalactites; ++i) 
            {
                f32 X = (f32)i / (f32)NumStalactites;
                f32 PositionFactor = -4*Square(X) + 4*X;
                DrawTriangle(Target, Stalactite * -(RandRange(18.0f, 42.0f)  + (1.0f - PositionFactor) * 10.0f)
                             + V2((X - 0.5f) * 400.0f, -310.0f - PositionFactor * 60.0f), 
                             HSVtoRGB(CaveColor));
            }
        } break;

        case BT_Field:
        {

        } break;

        case BT_Mountain:
        {

        } break;
    };
}

extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    if (!Memory->IsInitialized) { return; }

    game_state* GameState = (game_state*)Memory->PermanentBlock;
    if (!GameState->IsInitialized) {
        GameState->Seed = Input->MonotonicCounter;

        GameState->IsInitialized = true;
    }

    temp_state* TempState = (temp_state*)Memory->TemporaryBlock;
    if (!TempState->IsInitialized) {
        InitializeArena(&TempState->TempArena, 
                        Memory->TemporarySize - sizeof(temp_state), 
                        (void*)((u8*)Memory->TemporaryBlock + sizeof(temp_state)));

        TempState->DragonBitmap = {};
        TempState->DragonBitmap.Width = 512/GlobalRenderScale;
        TempState->DragonBitmap.Height = 512/GlobalRenderScale;
        TempState->DragonBitmap.BytesPerPixel = 4;
        TempState->DragonBitmap.Memory = PushSize(&TempState->TempArena, 
                                       TempState->DragonBitmap.Width * TempState->DragonBitmap.Height * TempState->DragonBitmap.BytesPerPixel); 

        TempState->BackgroundBitmap = {};
        TempState->BackgroundBitmap.Width = 512/GlobalRenderScale;
        TempState->BackgroundBitmap.Height = 512/GlobalRenderScale;
        TempState->BackgroundBitmap.BytesPerPixel = 4;
        TempState->BackgroundBitmap.Memory = PushSize(&TempState->TempArena, 
                                       TempState->BackgroundBitmap.Width * TempState->BackgroundBitmap.Height * TempState->BackgroundBitmap.BytesPerPixel); 

        SeedRand(GameState->Seed);
        ClearBitmap(&TempState->BackgroundBitmap, HEX_RGB(0x00111111));
        ClearBitmap(&TempState->DragonBitmap, HEX_RGB(0x00111111));
        DrawBackground(&TempState->BackgroundBitmap, FromU8Color(HEX_RGB(0x00111111)), 0.0f);
        DrawDragon(&TempState->DragonBitmap, FromU8Color(HEX_RGB(0x00111111)), 0.0f);

        TempState->IsInitialized = true;
    }

    if (Input->Reloaded) {
        GameState->Seed = (Input->MonotonicCounter * 345127) % 39187;
        SeedRand(GameState->Seed);
        ClearBitmap(&TempState->DragonBitmap, HEX_RGB(0x00111111));
        DrawBackground(&TempState->BackgroundBitmap, FromU8Color(HEX_RGB(0x00111111)), 0.0f);
        DrawDragon(&TempState->DragonBitmap, FromU8Color(HEX_RGB(0x00111111)), Input->MonotonicCounter/20.0f);
    }

    ClearBuffer(Buffer, HEX_RGB(0x00111111));
    ivec2 Offset = (Buffer->Dim - TempState->DragonBitmap.Dim*GlobalRenderScale)/2;
    DrawBitmap(Buffer, TempState->BackgroundBitmap, (int)Offset.X, (int)Offset.Y, GlobalRenderScale);
    DrawBitmap(Buffer, TempState->DragonBitmap, (int)Offset.X, (int)Offset.Y, GlobalRenderScale);
}
