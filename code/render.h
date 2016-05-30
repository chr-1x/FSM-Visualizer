#pragma once
#include "graphgen.h"
#include "geom.hpp"
#include "math.hpp"

typedef vec4 rgba_color;
struct bitmap

{
    union {
        struct {
            int Width, Height;
        };
        ivec2 Dim;
    };
    int BytesPerPixel;
    int Stride;
    void* Memory;
};

void ClearBitmap(bitmap* Image, rgba_color Color = V4(0,0,0,0));

void DrawBitmap(bitmap* Target, bitmap Image, int OffsetX = 0, int OffsetY = 0, int Scale = 3);

void DrawOval(app_state* State, bitmap* Buffer, vec2 Center, vec2 Radius, rgba_color Color, bool Blend = true);

void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         rgba_color Color, bool Blend = true);

void DrawTriangle(app_state* State, bitmap* Buffer, tri Tri, rgba_color Color, bool Blend = true);

void DrawQuad(app_state* State, bitmap* Buffer, quad Quad, rgba_color Color, bool Blend = true);

void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         f32 Thickness, rgba_color Color, bool Blend = true);

void DrawLinearArrow(app_state* State, bitmap* Buffer, vec2* Vertices, 
         f32 Thickness, f32 ArrowSize, rgba_color Color, bool Blend = true);

void DrawBezierQuadraticSegment(app_state* Start, bitmap* Target, int CurveCount, verts<3>* Curves, 
                            f32 TotalLength, f32 Thickness, rgba_color Color, 
                            int SegmentsPerCurve = 10, bool MiterEnds = true);

void DrawBezierCubicSegment(app_state* State, bitmap* Target, 
                            int CurveCount, verts<4>* Curves, f32 TotalLength,
                            f32 Thickness, rgba_color Color, 
                            int SegmentsPerCurve = 10, bool MiterEnds = true);

int DrawCharacterGlyph(app_state* State, 
                   bitmap* Target, int Character, vec2 Position, 
                   f32 HeightInUnits, rgba_color Color);

void DrawString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInPixels, rgba_color Color);

void DrawWorldString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInUnits, rgba_color Color);
