/* render.h
 * by Andrew Chronister, (c) 2016
 *
 * Declares a number of functions for drawing geometric primitives to bitmaps,
 * as well as the shared bitmap structure itself. */
#pragma once

// Purpose: brings in the app_state structure that contains the PixelsPerUnit
// conversion and the stb_truetype font information.
#include "graphgen.h"

// Purpose: geometric data structure definitions and utility functions
#include "geom.hpp"

// Purpose: bezier curve data structure definitions
#include "bezier.hpp"

// Purpose: vector structure definitions and utility functions
#include "math.hpp"

/* Alias for a 4-component vector of float, to clarify usage. */
typedef vec4 rgba_color;

/* Structure defining a rectangular section of memory used as a bitmap image. */
struct bitmap
{
    /* Anonymous union describing the dimensions of the bitmap in two different
     * ways for convenience. */
    union {
        struct {
            // Directly accessible width/height
            int Width, Height;
        };
        // Two-component vector describing the width and height as a single item
        ivec2 Dim;
    };
    // Number of bytes used to describe a single pixel. Often 4.
    int BytesPerPixel;
    // Distance from the start of one row of the bitmap to the start of the
    // next. Usually computable as Width * BytesPerPixel, but these can differ
    // if the bitmap is a subsection of a larger image.
    int Stride;
    // Pointer to the start of the bitmaps memory, specifically the first pixel
    // of the first row.
    void* Memory;
};

/* Drawing functions. 
 * For all of these:
 *  - Target is the bitmap to draw to
 *  - Color is the fill color of the geometric primitive
 *  - Blend specifies whether the function will perform alpha blending with the 
 *    existing contents of the Target bitmap. 
 *  - Positions and radii are given in world coordinates unless otherwise specified by
 *    function/argument names.
 *      - Translation is performed by first scaling by State->PixelsPerUnit, and
 *        then translating by the dimensions of the bitmap so that all visible
 *        parts of the bitmap are in positive coordinates.
 *  */

/* Fills every pixel in the bitmap with the given color, performing no blending. */
void ClearBitmap(bitmap* Target, rgba_color Color = V4(0,0,0,0));

/* Draws an oval onto Target centered at the given position, with major and
 * minor axes defined by the Radius vector. */
void DrawOval(app_state* State, bitmap* Target, vec2 Center, vec2 Radius, rgba_color Color, bool Blend = true);

/* Draws a straight anti-aliased line of 1 pixel width between StartP and EndP onto target. */
void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         rgba_color Color, bool Blend = true);

/* Draws a straight anti-aliased line with the given thickness between StartP and EndP onto target. */
void DrawLine(app_state* State, bitmap* Target, vec2 StartP, vec2 EndP,
         f32 Thickness, rgba_color Color, bool Blend = true);

/* Draws a line of the given thickness onto Target with an arrowhead at the end
 * point. */
void DrawLinearArrow(app_state* State, bitmap* Target, vec2 P1, vec2 P2,
         f32 Thickness, f32 ArrowSize, rgba_color Color, bool Blend = true);


/* Draws the filled solid-color triangle given by the coordinates of Tri. See
 * geom.hpp for the definition of tri. */
void DrawTriangle(app_state* State, bitmap* Target, tri Tri, rgba_color Color, bool Blend = true);

/* Draws the filled solid-color quad given by the coordinates of Quad. See
 * geom.hpp for the definition of quad. */
void DrawQuad(app_state* State, bitmap* Target, quad Quad, rgba_color Color, bool Blend = true);

/* Draws a single segment of a cubic bezier curve onto Target. See bezier.hpp
 * for the definition of bezier_cubic_segment.
 * LinesPerSegment specifies the number of line segments to use to approximate
 * the curve segment when drawing: more line segments will result in a smoother
 * appearance at the price of somewhat higher performance cost. */
void DrawBezierCubicSegment(app_state* State, bitmap* Target, 
                            bezier_cubic_segment Segment,
                            f32 Thickness, rgba_color Color, 
                            int LinesPerSegment = 10, bool MiterEnds = true);

/* Draws a single character glyph to Target using the codepoint given by
 * Character. 
 * NOTE(chronister): Position for this function is given in biased pixel
 *   coordinates (with origin at the center of the screen), NOT world unit coordinates. 
 *   Also, scale should be the scale given by stbtt_ScaleForPixelHeight, which will 
 *   vary per font. */
int DrawCharacterGlyph(app_state* State, 
                   bitmap* Target, int Character, vec2 PositionInPixels, 
                   f32 Scale, rgba_color Color);

/* Draws the ANSI string of length Length whose character data starts at the
 * pointer String to Target.
 * NOTE(chronister): Position for this function is given in biased pixel
 *   coordinates (with origin at the center of the screen). Height is given in
 *   pixels. */
void DrawString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInPixels, rgba_color Color);

/* Draws the ANSI string of length Length whose character data starts at the
 * pointer String to Target.
 * NOTE(chronister): Position for this function is given in world unit
 *   coordinates (with origin at the center of the screen). Height is given in
 *   units as well. */
void DrawWorldString(app_state* State, bitmap* Target, 
               size_t Length, char* String, 
               vec2 Position, f32 HeightInUnits, rgba_color Color);
