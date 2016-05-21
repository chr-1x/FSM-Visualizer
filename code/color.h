#pragma once
#include "random.h"

struct hsva_color
{
    f32 H;
    f32 S;
    f32 V;
    f32 A;
};

typedef vec4 rgba_color;

//TODO(chronister): Optimize these?
internal rgba_color
HSVtoRGB(hsva_color HSV)
{
    while (HSV.H < 0.0f) { HSV.H += 360.0f; }
    HSV.H = FloatMod(HSV.H, 360.0f);
    float C = HSV.V * HSV.S;
    float X = C * (1 - AbsoluteValue(FloatMod(HSV.H / 60.0f, 2.0f) - 1.0f));
    float m = HSV.V - C;
    rgba_color RGBA = {};
    if      (0   <= HSV.H && HSV.H < 60.0)  { RGBA = V4(C + m, X + m, m, HSV.A); }
    else if (60  <= HSV.H && HSV.H < 120.0) { RGBA = V4(X + m, C + m, m, HSV.A); }
    else if (120 <= HSV.H && HSV.H < 180.0) { RGBA = V4(m, C + m, X + m, HSV.A); }
    else if (180 <= HSV.H && HSV.H < 240.0) { RGBA = V4(m, X + m, C + m, HSV.A); }
    else if (240 <= HSV.H && HSV.H < 300.0) { RGBA = V4(X + m, m, C + m, HSV.A); }
    else if (300 <= HSV.H && HSV.H <= 360.0) { RGBA = V4(C + m, m, X + m, HSV.A); }

    return RGBA;
};

internal hsva_color
RGBtoHSV(rgba_color RGBA)
{
    float Cmax = Max(Max(RGBA.r, RGBA.g), RGBA.b);
    float Cmin = Min(Min(RGBA.r, RGBA.g), RGBA.b);
    float Delta = Cmax - Cmin;

    hsva_color HSVA = {};
    if (Delta < 0.0001f) { HSVA.H = 0.0f; }
    else if (Cmax == RGBA.r) { HSVA.H = 60.0f * FloatMod((RGBA.g - RGBA.b) / Delta, 6.0f); }
    else if (Cmax == RGBA.g) { HSVA.H = 60.0f * ((RGBA.b - RGBA.r) / Delta + 2.0f); }
    else if (Cmax == RGBA.b) { HSVA.H = 60.0f * ((RGBA.r - RGBA.g) / Delta + 4.0f); }

    if (Cmax < 0.0001f) { HSVA.S = 0.0f; }
    else { HSVA.S = Delta / Cmax; }

    HSVA.V = Cmax;
    HSVA.A = RGBA.a;
    return HSVA;
}

inline pixel_color
TO_U8_COLOR(hsva_color HSV)
{
    return TO_U8_COLOR(HSVtoRGB(HSV));
}

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

internal hsva_color
RandHSV(range HueRange, range SatRange, range ValRange) {
    hsva_color Result = {};

    Result.H = ModRange(RandRange(HueRange), 360.0f);
    Result.S = RandRange(SatRange);
    Result.V = RandRange(ValRange);

    Result.A = 1.0f;
    return Result;
}

inline hsva_color
DeltaHSV(hsva_color Src, f32 DeltaH, f32 DeltaS, f32 DeltaV)
{
    hsva_color Dest = Src;
    Dest.H = FloatMod(Src.H + DeltaH, 360.0f);
    Dest.S = Max(Min(Src.S + DeltaS, 1.0f), 0.0f);
    Dest.V = Max(Min(Src.V + DeltaV, 1.0f), 0.0f);
    return Dest;
}

inline hsva_color
DeltaV(hsva_color Src, f32 DeltaV)
{
    return DeltaHSV(Src, 0, 0, DeltaV);
}
