#include "graphgen.h"
#include "render.h"
#include "geom.hpp"
#include "bezier.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

inline rgba_color
FromU8Color(pixel_color Src)
{
    rgba_color Dest = {};
    Dest.r = Src.R / 255.0f;
    Dest.g = Src.G / 255.0f;
    Dest.b = Src.B / 255.0f;
    Dest.a = Src.A / 255.0f;
    return Dest;
}

void ClearBitmap(bitmap* Image, rgba_color ClearColor)
{
    pixel_color Color = TO_U8_COLOR(ClearColor);
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

void DrawBitmap(bitmap* Buffer, bitmap Image, int OffsetX, int OffsetY, int Scale)
{
    int DestMinX = Max(OffsetX, 0);
    int DestMinY = Max(OffsetY, 0);
    int DestMaxX = Min(OffsetX + Scale*Image.Width, Buffer->Width);
    int DestMaxY = Min(OffsetY + Scale*Image.Height, Buffer->Height);
    
    for (int Y = DestMinY; Y < DestMaxY; ++Y)
    {
        uint8* SourceRow = (uint8*)Image.Memory + (Image.Width * Image.BytesPerPixel) * ((Y - DestMinY)/Scale);
        uint8* DestRow = (uint8*)Buffer->Memory + (Buffer->Stride) * Y;
        for (int X = DestMinX; X < DestMaxX; ++X)
        {
            pixel_color* SourcePixel = (pixel_color*)(SourceRow + (X - DestMinX)/Scale * Image.BytesPerPixel);
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * Buffer->BytesPerPixel);

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

void DrawOval(app_state* State, bitmap* Buffer, vec2 Center, vec2 Radius, rgba_color Color, bool Blend)
{
    Color.rgb *= Color.a;
    pixel_color SrcColor = TO_U8_COLOR(Color);

    Center = (Center * State->PixelsPerUnit) + (vec2)Buffer->Dim / 2.0f;
    Radius = Radius * State->PixelsPerUnit;
    int MinX = (int)Max(Center.X - Radius.X, 0);
    int MinY = (int)Max(Center.Y - Radius.Y, 0);
    int MaxX = (int)Min(Center.X + Radius.X + 1, Buffer->Width);
    int MaxY = (int)Min(Center.Y + Radius.Y + 1, Buffer->Height);

    for (int Y = MinY; Y < MaxY; ++Y) 
    {
        uint8* DestRow = (uint8*)Buffer->Memory + (Buffer->Width * Buffer->BytesPerPixel) * Y;
        for (int X = MinX; X < MaxX; ++X)
        {
            pixel_color* DestPixel = (pixel_color*)(DestRow + X * Buffer->BytesPerPixel);
            vec2 Dir = V2(X, Y) - Center;

            f32 PixRadius = Square(((f32)X - Center.X) / Radius.X) + Square(((f32)Y - Center.Y) / Radius.Y);
            f32 RadiusSpacePix = 2.0f / Radius.X;
            if (PixRadius < 1.0f - RadiusSpacePix)
            {
                // Fill the pixel
                if (Blend) {
                    *DestPixel = (*DestPixel*(1.0f - Color.a) + SrcColor);
                } else {
                    *DestPixel = SrcColor;
                }
            }
            else if (PixRadius <= (1.0f))
            {
                f32 dAlpha = Lerp(1.0f, Unlerp(1.0f - RadiusSpacePix, PixRadius, 1.0f), 0.0f);
                pixel_color dColor = SrcColor * dAlpha;
                if (Blend) {
                    *DestPixel = (*DestPixel*(1.0f - dAlpha*Color.a) + dColor);
                } else {
                    *DestPixel = dColor;
                }
            }
        }
    }
}

inline void
FillPixel(bitmap* Target, int X, int Y, rgba_color Color, f32 Alpha, bool Blend = true)
{
    u8* DestRow = (u8*)Target->Memory + (Target->Width * Target->BytesPerPixel) * Y;
    pixel_color* DestPixel = (pixel_color*)(DestRow + (Target->BytesPerPixel * X));
    rgba_color DestColor = FromU8Color(*DestPixel);
    if (Blend)
    {
        DestColor = (1.0f - Alpha) * DestColor + Alpha * Color;
    }
    else
    {
        DestColor = Alpha * Color;
    }
    *DestPixel = TO_U8_COLOR(DestColor);
}

void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         rgba_color Color, bool Blend)
{
    // Xioalin Wu's algorithm
    StartP = StartP * State->PixelsPerUnit + 0.5f*Target->Dim;
    EndP = EndP * State->PixelsPerUnit + 0.5f*Target->Dim;

    // Need a 1 pixel buffer on the edges because the line algo assumes it
    // can write 1 pixel over from the "max" and min
    vec2 Min = Clamp(V2(0,0), StartP, Target->Dim - V2(1,1));
    vec2 Max = Clamp(V2(0,0), EndP, Target->Dim - V2(1,1));

    bool Steep = AbsoluteValue(Max.y - Min.y) > AbsoluteValue(Max.x - Min.x);
    if (Steep) 
    {
        Min = SwapComponents(Min);
        Max = SwapComponents(Max);
    }
    if (Min.X > Max.X)
    {
        vec2 Temp = Min;
        Min = Max;
        Max = Temp;
    }

    vec2 Delta = Max - Min;
    f32 Gradient = SafeRatio0(Delta.y, Delta.x);

    // First endpoint
    vec2 End = V2((f32)Round(Min.x), Min.y + Gradient * (Round(Min.x) - Min.x));
    f32 xGap = 1.0f - FractionalPart(Min.x + 0.5f);
    vec2 Pixel1 = V2(End.x, (f32)Truncate(End.y));
    if (Steep)
    {
        FillPixel(Target, (int)Pixel1.y, (int)Pixel1.x, Color, (1.0f - FractionalPart(End.y)) * xGap, Blend);
        FillPixel(Target, (int)Pixel1.y + 1, (int)Pixel1.x, Color, FractionalPart(End.y) * xGap, Blend);
    }
    else
    {
        FillPixel(Target, (int)Pixel1.x, (int)Pixel1.y, Color, (1.0f - FractionalPart(End.y)) * xGap, Blend);
        FillPixel(Target, (int)Pixel1.x, (int)Pixel1.y + 1, Color, FractionalPart(End.y) * xGap, Blend);
    }
    f32 IntersectionY = End.y + Gradient;

    // Second endpoint
    End = V2((f32)Round(Max.x), Max.y + Gradient * ((f32)Round(Max.x) - Max.x));
    xGap = 1.0f - FractionalPart(Max.x + 0.5f);
    vec2 Pixel2 = V2(End.x, (f32)Truncate(End.y));
    if (Steep)
    {
        FillPixel(Target, (int)Pixel2.y, (int)Pixel2.x, Color, (1.0f - FractionalPart(End.y)) * xGap, Blend);
        FillPixel(Target, (int)Pixel2.y + 1, (int)Pixel2.x, Color, FractionalPart(End.y) * xGap, Blend);
    }
    else
    {
        FillPixel(Target, (int)Pixel2.x, (int)Pixel2.y, Color, (1.0f - FractionalPart(End.y)) * xGap, Blend);
        FillPixel(Target, (int)Pixel2.x, (int)Pixel2.y + 1, Color, FractionalPart(End.y) * xGap, Blend);
    }

    // Main loop
    for (f32 X = Pixel1.x + 1; X < Pixel2.x; ++X)
    {
        if (Steep)
        {
            FillPixel(Target, (int)IntersectionY, (int)X, Color, (1.0f - FractionalPart(IntersectionY)), Blend);
            FillPixel(Target, (int)IntersectionY + 1, (int)X, Color, FractionalPart(IntersectionY), Blend);
        }
        else
        {
            FillPixel(Target, (int)X, (int)IntersectionY, Color, (1.0f - FractionalPart(IntersectionY)), Blend);
            FillPixel(Target, (int)X, (int)IntersectionY + 1, Color, FractionalPart(IntersectionY), Blend);
        }
        IntersectionY += Gradient;
    }
}

void DrawTriangle(app_state* State, bitmap* Buffer, tri Tri, rgba_color Color, bool Blend)
{
    Color.rgb *= Color.a;
    pixel_color SrcColor = TO_U8_COLOR(Color);

    // "Anti aliasing"
    //  lean on the line drawing algo to outline the triangle, since it's all one color.
    DrawLine(State, Buffer, Tri.Verts[0], Tri.Verts[1], Color, Blend);
    DrawLine(State, Buffer, Tri.Verts[1], Tri.Verts[2], Color, Blend);
    DrawLine(State, Buffer, Tri.Verts[2], Tri.Verts[0], Color, Blend);

    Tri = (Tri * State->PixelsPerUnit) + (vec2)Buffer->Dim / 2.0f;

    int MinX = (int)Max(Min(Min(Tri.Verts[0].X, Tri.Verts[1].X), Tri.Verts[2].X), 0);
    int MaxX = (int)Min(Max(Max(Tri.Verts[0].X, Tri.Verts[1].X), Tri.Verts[2].X), Buffer->Width);
    int MinY = (int)Max(Min(Min(Tri.Verts[0].Y, Tri.Verts[1].Y), Tri.Verts[2].Y), 0);
    int MaxY = (int)Min(Max(Max(Tri.Verts[0].Y, Tri.Verts[1].Y), Tri.Verts[2].Y), Buffer->Height);

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

void DrawQuad(app_state* State, bitmap* Buffer, quad Quad, rgba_color Color, bool Blend)
{
    tri Tri1 = { Quad.Verts[0], Quad.Verts[1], Quad.Verts[2] };
    tri Tri2 = { Quad.Verts[2], Quad.Verts[3], Quad.Verts[0] };
    DrawTriangle(State, Buffer, Tri1, Color, Blend);
    DrawTriangle(State, Buffer, Tri2, Color, Blend);
}

//TODO(chronister): Better way to use the algorithm to do thick lines?
void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         f32 Thickness, rgba_color Color, bool Blend)
{
    Thickness = Thickness * State->PixelsPerUnit;
    vec2 Diff = EndP - StartP;
    vec2 Normal = LeftNormal(Diff);
    f32 Spacing = 0.8f;
    for (f32 i = -Thickness/2.0f; i <= Thickness/2.0f; i += Spacing)
    {
        f32 PixelDist = Spacing / State->PixelsPerUnit;
        vec2 OffStartP = StartP + PixelDist*Normal*i;
        vec2 OffEndP = EndP + PixelDist*Normal*i;
        DrawLine(State, Target, OffStartP, OffEndP, Color, Blend);
    }
}

void DrawLinearArrow(app_state* State, bitmap* Buffer, vec2* Vertices, 
         f32 Thickness, f32 ArrowSize, rgba_color Color, bool Blend)
{
    vec2 P1 = Vertices[0];
    vec2 P2 = Vertices[1];

    vec2 Diff = P2 - P1;
    P2 = P1 + (Length(Diff) - ArrowSize)*Normalize(Diff);

    tri Arrowhead;
    Arrowhead.Verts[0] = P2 + ArrowSize*LeftNormal(Diff);
    Arrowhead.Verts[1] = P2 + ArrowSize*Normalize(Diff);
    Arrowhead.Verts[2] = P2 + ArrowSize*RightNormal(Diff);

    DrawLine(State, Buffer, P1, P2, Thickness, Color, Blend);
    DrawTriangle(State, Buffer, Arrowhead, Color, Blend);
}

void DrawBezierQuadraticSegment(app_state* State, bitmap* Target, 
                            int CurveCount, verts<3>* Curves, f32 TotalLength,
                            f32 Thickness, rgba_color Color, 
                            int SegmentsPerCurve, bool MiterEnds)
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

            DrawLine(State, Target, Line[0], Line[1], Thickness, Color, 
                     MiterEnds || (0 < SegmentIndex && SegmentIndex < SegmentsPerCurve - 1) || (0 < CurveIndex && CurveIndex < CurveCount - 1));

            Percent = NextPercent;
        }
    }
}

void DrawBezierCubicSegment(app_state* State, bitmap* Target, 
                            int CurveCount, verts<4>* Curves, f32 TotalLength,
                            f32 Thickness, rgba_color Color, 
                            int SegmentsPerCurve, bool MiterEnds)
{
    //TODO(chronister): Is there a good way to approximate this length more quickly?
    f32 Percent = 0.0f;
    for (int CurveIndex = 0; CurveIndex < CurveCount; ++CurveIndex)
    {
        verts<4> CurveSegment = Curves[CurveIndex];
        for (int SegmentIndex = 0; SegmentIndex < SegmentsPerCurve; ++SegmentIndex)
        {
            f32 t = (f32)SegmentIndex / (f32)SegmentsPerCurve;
            f32 tNext = (f32)(SegmentIndex + 1) / (f32)SegmentsPerCurve;
            
            vec2 Line[2];
            Line[0] = BezierCubic(CurveSegment.Verts[0],
                                  CurveSegment.Verts[1],
                                  CurveSegment.Verts[2],
                                  CurveSegment.Verts[3], t);
            Line[1] = BezierCubic(CurveSegment.Verts[0],
                                  CurveSegment.Verts[1],
                                  CurveSegment.Verts[2],
                                  CurveSegment.Verts[3], tNext);
            f32 SegmentLength = Length(Line[1] - Line[0]);
            f32 NextPercent = Percent + (SegmentLength / TotalLength);

            DrawLine(State, Target, Line[0], Line[1], Thickness, Color, 
                     MiterEnds || (0 < SegmentIndex && SegmentIndex < SegmentsPerCurve - 1) || (0 < CurveIndex && CurveIndex < CurveCount - 1));

            Percent = NextPercent;
        }
    }
}


int DrawCharacterGlyph(app_state* State, 
                   bitmap* Target, int Character, vec2 Position, 
                   f32 Scale, rgba_color Color)
{
    if (isnan(Position.x) || isnan(Position.y)) { return 0; }

    temporary_memory GlyphMemory = BeginTemporaryMemory(&State->TempArena);

    int GlyphX1 = 0, 
        GlyphY1 = 0, 
        GlyphX2 = 0, 
        GlyphY2 = 0;

    stbtt_GetCodepointBitmapBox(&State->FontInfo, Character, 
                                Scale, Scale,
                                &GlyphX1, &GlyphY1, &GlyphX2, &GlyphY2);

    int GlyphW = GlyphX2 - GlyphX1;
    int GlyphH = GlyphY2 - GlyphY1;
    // Need an apron so that we can do a bilinear interp later
    int WidthAndApron = GlyphW + 2;
    int HeightAndApron = GlyphH + 3;

    u8* MonoBitmap = (u8*)PushSize(&State->TempArena, WidthAndApron*HeightAndApron);
    memset(MonoBitmap, 0, WidthAndApron*HeightAndApron);

    stbtt_MakeCodepointBitmap(&State->FontInfo, (MonoBitmap + WidthAndApron + 1), // Go down 1 row and col
                             WidthAndApron - 2, HeightAndApron - 2, WidthAndApron,
                             Scale, Scale,
                             Character);

    int MonoBytesPerPixel = 1;

    Position = Position + 0.5f*Target->Dim;
    // Take into account the position that the glyph told us it should be drawn
    Position.y -= GlyphY2;

    vec2 GlyphHalfDim = V2(GlyphW / 2.0f, (GlyphH + 1) / 2.0f);

    // Bounds are calculated from the baseline and left side, since the font metrics
    //  assume it'll be positioned as such.
    int MinX = (int)(Position.x);
    int MaxX = (int)((Position + 2*GlyphHalfDim).x);
    int MinY = (int)(Position.y);
    int MaxY = (int)((Position + 2*GlyphHalfDim).y);

    int ClampedMinX = Max(MinX, 0);
    int ClampedMinY = Max(MinY, 0);
    int ClampedMaxX = Min(MaxX, Target->Width);
    int ClampedMaxY = Min(MaxY, Target->Height);

    int SourcePitch = (WidthAndApron * MonoBytesPerPixel);

    for(int Y = ClampedMinY; Y < ClampedMaxY; ++Y)
    {
        f32 RefY = ((Position + 2*GlyphHalfDim).y - Y - 1.5f);
        u8 *SourceRow = MonoBitmap + SourcePitch * (Floor(RefY) + 1);
        u8 *DestRow = (u8*)Target->Memory + (Target->Width * Target->BytesPerPixel) * (Y);
        for(int X = ClampedMinX; X < ClampedMaxX; ++X)
        {
            vec2 Ref = V2(X + 1.5f - Position.x, RefY);
            vec2 Error = Ref - V2(Floor(Ref.x), Floor(Ref.y));

            // Grab out the four pixels to sample from the glyph
            u8 MonoColor1 = *(SourceRow + MonoBytesPerPixel*(Floor(Ref.x)));
            u8 MonoColor2 = *(SourceRow + MonoBytesPerPixel*(Floor(Ref.x + 1)));
            u8 MonoColor3 = *(SourceRow + SourcePitch + MonoBytesPerPixel*(Floor(Ref.x)));
            u8 MonoColor4 = *(SourceRow + SourcePitch + MonoBytesPerPixel*(Floor(Ref.x + 1)));

            pixel_color* DestPixel = (pixel_color*)(DestRow + (Target->BytesPerPixel * X));
            rgba_color DestColor = FromU8Color(*DestPixel);

            // Do a bilinear interpolation on the four source pixels
            u8 MonoColor = (u8)Lerp(Lerp(MonoColor1, Error.x, MonoColor2), 
                                    Error.y, 
                                    Lerp(MonoColor3, Error.x, MonoColor4));

            f32 Alpha = (f32)MonoColor/255.0f;

            DestColor.r = (1.0f - Alpha) * DestColor.r + Alpha * Color.r;
            DestColor.g = (1.0f - Alpha) * DestColor.g + Alpha * Color.g;
            DestColor.b = (1.0f - Alpha) * DestColor.b + Alpha * Color.b;
            DestColor.a = 1.0f;

            *DestPixel = TO_U8_COLOR(DestColor);
        }
    }

    EndTemporaryMemory(GlyphMemory);

    return GlyphW;
}

internal f32
CalculateStringWidth(app_state* State, 
                     size_t Length, char* String, f32 HeightInPixels)
{
    f32 TotalWidth = 0.0f;
    f32 Scale = stbtt_ScaleForPixelHeight(&State->FontInfo, HeightInPixels);
    for (int CharIndex = 0; CharIndex < Length; ++CharIndex)
    {
        int Advance, LeftSideBearing, KernAdvance = 0;
        stbtt_GetCodepointHMetrics(&State->FontInfo, String[CharIndex],
                                   &Advance, &LeftSideBearing);
        if (CharIndex < Length - 1)
        {
            KernAdvance = stbtt_GetCodepointKernAdvance(&State->FontInfo, String[CharIndex - 1], String[CharIndex]);
        }

        TotalWidth += (Advance + KernAdvance) * Scale;
    }
    return TotalWidth;
}

void DrawString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInPixels, rgba_color Color)
{
    f32 TotalWidth = CalculateStringWidth(State, Length, String, HeightInPixels);
    f32 XOffset = -TotalWidth/2.0f;
    f32 Scale = stbtt_ScaleForPixelHeight(&State->FontInfo, HeightInPixels);

    for (int CharIndex = 0; CharIndex < Length; ++CharIndex)
    {
        int Advance, LeftSideBearing, KernAdvance = 0;
        stbtt_GetCodepointHMetrics(&State->FontInfo, String[CharIndex],
                                   &Advance, &LeftSideBearing);

        DrawCharacterGlyph(State, Target, String[CharIndex], 
                           Position + V2(XOffset, 0.0f), 
                           Scale, Color);


        XOffset += Advance * Scale;

        if (CharIndex < Length - 1)
        {
            KernAdvance = stbtt_GetCodepointKernAdvance(&State->FontInfo, String[CharIndex], String[CharIndex + 1]);

            XOffset += KernAdvance * Scale;
        }

    }
}

void DrawWorldString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInUnits, rgba_color Color)
{
    DrawString(State, Target, Length, String,
               (Position - V2(0.0f, 0.25f*HeightInUnits)) * State->PixelsPerUnit,
               HeightInUnits * State->PixelsPerUnit, Color);
}
