#include <stdarg.h>
#include "chr.h"
#include "chr_gamemath.h"
#include "dracogen.h"
#include "color.h"
#include "random.h"
#include "geom.h"
#include "bezier.h"

global_variable f32 GlobalPixelsPerUnit = 2.95f;
global_variable f32 GlobalRenderScale = 3.0f;

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

            DestPixel->to.R = SourcePixel->R;
            DestPixel->to.G = SourcePixel->G;
            DestPixel->to.B = SourcePixel->B;
            DestPixel->to.A = SourcePixel->A;
        }
    }
}

internal void
DrawOvalSlice(bitmap* Buffer, vec2 Center, vec2 Radius, pixel_color Color, vec2 SliceStartDir, vec2 SliceEndDir)
{
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
                *DestPixel = Color;
            }
        }
    }
}

internal void
DrawOval(bitmap* Buffer, vec2 Center, vec2 Radius, pixel_color Color) 
{
    return DrawOvalSlice(Buffer, Center, Radius, Color, V2(1, 0), V2(1, 0));
}


internal void
DrawTriangle(bitmap* Buffer, tri Tri, pixel_color Color)
{
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
                *DestPixel = Color;
            }
        }
    }
}

internal void
DrawQuad(bitmap* Buffer, quad Quad, pixel_color Color)
{
    tri Tri1 = { Quad.Verts[0], Quad.Verts[1], Quad.Verts[2] };
    tri Tri2 = { Quad.Verts[2], Quad.Verts[3], Quad.Verts[0] };
    DrawTriangle(Buffer, Tri1, Color);
    DrawTriangle(Buffer, Tri2, Color);
}

template<int N>
internal void
DrawPoly(bitmap* Buffer, verts<N> Poly, pixel_color Color)
{
    for (int VertIndex = 2; VertIndex < N; ++VertIndex)
    {
        // Convex polyhedron
        tri Tri = { Poly.Verts[0], Poly.Verts[VertIndex - 1], Poly.Verts[VertIndex] };
        DrawTriangle(Buffer, Tri, Color);
    }
}

internal void
DrawLine(bitmap* Buffer, int NumVertices, vec2* Vertices, f32 (*Thickness)(f32 Percent, void* Param), pixel_color Color, bool32 MiterEnds=true, void* Param = NULL)
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

        DrawQuad(Buffer, SegmentQuad, Color);

        if (MiterEnds && i == 1) 
        {
            DrawOvalSlice(Buffer, Start, V2(StartThickness/2,StartThickness/2), Color,
                          LeftNormal, RightNormal);
        }

        if (i < NumVertices - 1)
        {
            vec2 NextSegment = Vertices[i + 1] - Vertices[i];

            vec2 NextLeftOrtho = V2(NextSegment.Y, -NextSegment.X);
            vec2 NextRightOrtho = V2(-NextSegment.Y, NextSegment.X);

            if (Inner(RightNormal, NextRightOrtho) != Inner(RightNormal,RightNormal)) {
                DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color, 
                              RightNormal, NextRightOrtho);
            }

            if (Inner(LeftNormal, NextLeftOrtho) != Inner(RightNormal,RightNormal)) {
                DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color, 
                              LeftNormal, NextLeftOrtho);
            }
        }

        if (MiterEnds)
        {
            DrawOvalSlice(Buffer, End, V2(EndThickness/2,EndThickness/2), Color,
                          LeftNormal, RightNormal);
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
DrawLine(bitmap* Buffer, int NumVertices, vec2* Vertices, f32 (*Thickness)(f32 Percent), pixel_color Color, bool32 MiterEnds=true)
{
    return DrawLine(Buffer, NumVertices, Vertices, ThicknessFuncWrapper, Color, MiterEnds, (void*)Thickness);
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
                       f32 (*Thickness)(f32 Percent), pixel_color Color, 
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
    return SnootThicknessScale * Percent + SnootThicknessBase;
}

internal f32
SnootTopThickness(f32 Percent)
{
    return SnootPartsProportion * SnootThickness(Percent);
}

internal f32
SnootBottomThickness(f32 Percent)
{
    return (1.0f - SnootPartsProportion) * SnootThickness(Percent);
}

internal vec2
SnootTopOffset(f32 Percent)
{
    return V2((1.0f - Percent) * -10.0f, (SnootThickness(Percent) - SnootTopThickness(Percent)) / 2 + 
              SnootMouthSize * SnootThicknessBase * Percent);
}

internal vec2
SnootBottomOffset(f32 Percent)
{
    return V2(SnootMouthOverbite * Percent, -(SnootThickness(Percent) - SnootBottomThickness(Percent)) / 2 +
              -SnootMouthSize * SnootThicknessBase * Percent);
}

internal f32
MouthThickness(f32 Percent)
{
    return (1.0f - Percent) * SnootBottomThickness(Percent);
}

internal vec2
MouthOffset(f32 Percent)
{
    return V2(SnootMouthOverbite * Percent, 
              SnootBottomOffset(Percent).Y + SnootBottomThickness(Percent) * 0.5f + MouthThickness(Percent) * 0.5f);
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
static vec2 LastSecondaryNeckPlatePos = V2(0,0);
static vec2 LastNeckSpinePos = V2(0,0);
static f32 MinNeckPlateDist = 30.0f;
static f32 MinNeckSpineDist = 30.0f;
static vec2 HeadPosRef = V2(0,0);

internal verts<5>
NeckPlateTransformFunc(verts<5> Plate, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position + NeckThickness(Percent) * 0.42f * Normalize(V2(-Direction.Y, Direction.X));
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
NeckSecondaryPlateTransformFunc(verts<5> Plate, f32 Percent, vec2 Direction, vec2 Position)
{
    vec2 TranslatePos = Position + NeckThickness(Percent) * 0.22f * Normalize(V2(-Direction.Y, Direction.X));
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
    vec2 TranslatePos = Position - NeckThickness(Percent) * 0.49f * Normalize(V2(-Direction.Y, Direction.X));
    if ((Percent < 0.01f || Length(TranslatePos - LastNeckSpinePos) > MinNeckSpineDist))
    {
        LastNeckSpinePos = TranslatePos;

        Spine = Spine * 5.0f;
        Spine = RotateAroundOrigin(Spine, PI32 + ATan2(Direction.Y, Direction.X));
        Spine = Spine + TranslatePos;
        return Spine;
    }

    //TODO(chronister): HACK!
    return Spine + V2(-1000, -1000);
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
DrawDragon(bitmap* Target, pixel_color BGColor = HEX_RGB(0x000000))
{
    hsva_color ScaleColor = RandHSV(BETWEEN(0.0f, 360.0f), BETWEEN(0.3f, 0.7f), BETWEEN(0.3f, 0.6f));
    hsva_color EyeColor = RandHSV(AROUND(ScaleColor.H, 90), BETWEEN(0.7f, 1.0f), BETWEEN(0.6f, 0.8f));
    hsva_color FinColor = RandHSV(BETWEEN(0.0f, 360.0f), BETWEEN(0.4f, 0.8f), BETWEEN(0.4f, 0.6f));
    hsva_color HornColor = RandHSV(BETWEEN(0.0f, 360.0f), BETWEEN(0.1f, 0.4f), BETWEEN(0.4f, 0.6f));
    hsva_color MouthColor = RandHSV(AROUND(ScaleColor.H, 5), BETWEEN(0.4f, 0.8f), BETWEEN(0.2f, 0.4f));
    hsva_color ToothColor = RandHSV(AROUND(HornColor.H, 5), BETWEEN(0.0f, 0.3f), BETWEEN(0.8f, 1.0f));

    vec2 HeadDim = V2(RandRange(70, 85), RandRange(60, 75));
    vec2 HeadOffset = HeadPosRef = V2(RandRange(-20, 40), RandRange(-10, 20));

    SnootThicknessBase = HeadDim.Y * RandRange(1.2f, 1.6f);
    SnootThicknessScale = RandRange(-1, -10);
    SnootMouthSize = RandRange(0.0f, 0.3f);
    SnootMouthOverbite = RandRange(-20.0f, 2.0f);

    NeckThicknessBase = HeadDim.Y*2;
    NeckThicknessScale = RandRange(20, 50);

    vec2 EyeSize = V2(RandRange(20, 30), RandRange(10, 15));
    vec2 EyeOffset = V2(RandRange(10, 20), 15);

    vec2 FarEyeOffset = V2(HeadDim.X - 20, Min(SnootThicknessBase - HeadDim.Y - 1.5*EyeSize.Y, HeadDim.Y * 0.6f));

    vec2 NoseOffset = V2(20, RandRange(40, 20));

    vec2 HeadDir = V2(RandRange(10, 40), RandSign() * RandRange(10, 20));

    bezier_curve<3> Neck = CurveByOffsets<3>(
          HeadOffset, 
              -HeadDir,
              V2(RandRange(AROUND(-60, 30)), RandRange(-40, -10)),

              V2(RandRange(-100, -60), RandRange(AROUND(-50, 10))), V2(80, RandRange(AROUND(-100, 10))),

              V2(170, -100), V2(-200, -100));

    verts<4> Snoot = VerticesByOffsets<4>(V2(5,-HeadDim.Y + SnootThicknessBase/2),
                                          V2(40,RandRange(0, -2)),
                                          V2(RandRange(25, 35),RandRange(0, -5)),
                                          V2(RandRange(30, 50),RandRange(0, -5))) + HeadOffset;


    verts<5> Horn = VerticesByOffsets<5>(V2(-60, 30),
                                         V2(RandRange(-30, -50), RandRange(20, 40)),
                                         V2(RandRange(5, -50), RandRange(5, 50)),
                                         V2(-10, 10),
                                         V2(RandRange(-5, -25), RandRange(5, 20))) + HeadOffset;

    DrawLine(Target, ArrayCount(Horn.Verts), (Horn + V2(2*HeadDim.X/3, FarEyeOffset.Y)).Verts, 
             HornThickness, TO_U8_COLOR(DimValue(HornColor, 0.2f)), true);

#if 0
    DrawOval(Target, HeadOffset + EyeOffset + FarEyeOffset, EyeSize + V2(-7.5f, -1), TO_U8_COLOR(EyeColor));
    DrawOval(Target, HeadOffset + EyeOffset + FarEyeOffset, V2(4, EyeSize.Y), TO_U8_COLOR(EyeColor) * 0.2f);
#endif

    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           NeckThickness, TO_U8_COLOR(ScaleColor), 9, true);

    // Neck highlight
    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           NeckHighlightThickness, TO_U8_COLOR(DimValue(ScaleColor, -0.03f)), 9, true);

    DrawOval(Target, HeadOffset, HeadDim, TO_U8_COLOR(ScaleColor));


    int NumPlates = RandRange(15, 22);
    f32 PlateConcavityFactor = RandRange(-0.4f, 0.4f);
    for (int i = 0; i <= NumPlates; ++i)
    {
        verts<5> Plate = Vertices<5>(V2(PlateConcavityFactor, 0.0f), V2(0, -1), V2(-1.0f, -0.7f), V2(-1.0f, 0.7f), V2(0, 1));
        DrawPoly(Target, 
                 CurveTransformParametric(Neck, Plate, 0.08f + 0.92f*((f32)i / (f32)NumPlates), 
                                          NeckPlateTransformFunc),
                 TO_U8_COLOR(DimValue(HornColor, 0.2f)));

        if (i < NumPlates) {
            DrawPoly(Target, 
                     CurveTransformParametric(Neck, Plate, 0.08f + 0.92f*((f32)(i + 0.5f) / (f32)NumPlates), 
                                              NeckSecondaryPlateTransformFunc),
                     TO_U8_COLOR(DimValue(HornColor, 0.2f)));
        }
    }

    int NumSpines = 20;
    for (int i = 0; i <= NumSpines; ++i)
    {
        verts<5> Spine = Vertices<5>(V2(-3, 0), V2(4, 0), V2(2, 2), V2(-4, 3), V2(-3, 2));
        DrawPoly(Target, 
                 CurveTransformParametric(Neck, Spine, ((f32)i / (f32)NumSpines), NeckSpineTransformFunc),
                 TO_U8_COLOR(HornColor));

#if 0
        verts<1> Pt = Vertices<1>(V2(0,0));
        DrawOval(Target,
                 CurveTransformParametric(Neck, Pt, ((f32)i / (f32)NumSpines), NeckPlateTransformFunc).Verts[0],
                 V2(5,5), HEX_RGB((int)(((f32)i / (f32)NumSpines) * 255) << 16));
#endif
    }

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthClearOffset).Verts, 
             MouthClearThickness, BGColor, false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, MouthOffset).Verts, 
             MouthThickness, TO_U8_COLOR(MouthColor), false);
    for (int i = 0; i < ArrayCount(Snoot.Verts); ++i)
    {
        tri Tooth = Vertices<3>(V2(-1, 0), V2(0, (f32)SQRT3), V2(1, 0));
        DrawTriangle(Target, 
                     Tooth * 4 + 
                     LineOffsetParametric(Snoot, SnootBackToothOffset).Verts[i], 
                     TO_U8_COLOR(DimValue(ToothColor, 0.3f)));

        DrawTriangle(Target, 
                     Tooth * 6 + 
                     LineOffsetParametric(Snoot, SnootFrontToothOffset).Verts[i], 
                     TO_U8_COLOR(ToothColor));
    }

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootBottomOffset).Verts, 
             SnootBottomThickness, TO_U8_COLOR(ScaleColor), false);

    DrawLine(Target, ArrayCount(Snoot.Verts), LineOffsetParametric(Snoot, SnootTopOffset).Verts, 
             SnootTopThickness, TO_U8_COLOR(ScaleColor), false);

#if 0
    DrawBezierCurveSegment(Target, 3, (verts<3>*)&Neck.Segments, Neck.TotalLength,
                           TestThickness, HEX_RGB(0xFF0000), 9, true);
#endif


    DrawLine(Target, ArrayCount(Horn.Verts), Horn.Verts, HornThickness, TO_U8_COLOR(HornColor), true);

    DrawOval(Target, HeadOffset + EyeOffset, EyeSize, TO_U8_COLOR(EyeColor));
    DrawOval(Target, HeadOffset + EyeOffset, V2(5, EyeSize.Y), TO_U8_COLOR(DimValue(EyeColor, 0.8f)));

    DrawOval(Target, Snoot.Verts[2] + NoseOffset, V2(10, 3), TO_U8_COLOR(DimValue(ScaleColor, 0.8f)));

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
        TempState->DragonBitmap.Width = 768/GlobalRenderScale;
        TempState->DragonBitmap.Height = 768/GlobalRenderScale;
        TempState->DragonBitmap.BytesPerPixel = 4;
        TempState->DragonBitmap.Memory = PushSize(&TempState->TempArena, 
                                       TempState->DragonBitmap.Width * TempState->DragonBitmap.Height * TempState->DragonBitmap.BytesPerPixel); 

        SeedRand(GameState->Seed);
        ClearBitmap(&TempState->DragonBitmap, HEX_RGB(0x111111));
        DrawDragon(&TempState->DragonBitmap);

        TempState->IsInitialized = true;
    }

    if (Input->Reloaded) {
        GameState->Seed = (Input->MonotonicCounter * 345127) % 39187;
        SeedRand(GameState->Seed);
        ClearBitmap(&TempState->DragonBitmap, HEX_RGB(0x111111));
        DrawDragon(&TempState->DragonBitmap, HEX_RGB(0x111111));
    }

    ivec2 Offset = (Buffer->Dim - TempState->DragonBitmap.Dim*GlobalRenderScale)/2;
    DrawBitmap(Buffer, TempState->DragonBitmap, (int)Offset.X, (int)Offset.Y, GlobalRenderScale);
}
