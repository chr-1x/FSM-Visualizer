#pragma once
#include "random.h"

struct hsva_color
{
    f32 H,S,V,A;
};

union rgba_color
{
    struct { f32 R,G,B,A; };
    vec4 All;
};

//TODO(chronister): Optimize these?
internal rgba_color
HSVtoRGB(hsva_color HSV)
{
    float C = HSV.V * HSV.S;
    float X = C * (1 - AbsoluteValue(FloatMod(HSV.H / 60.0f, 2.0f) - 1.0f));
    float m = HSV.V - C;
    rgba_color RGBA = {};
    if      (0   <= HSV.H && HSV.H < 60.0)  { RGBA.All = V4(C + m, X + m, m, HSV.A); }
    else if (60  <= HSV.H && HSV.H < 120.0) { RGBA.All = V4(X + m, C + m, m, HSV.A); }
    else if (120 <= HSV.H && HSV.H < 180.0) { RGBA.All = V4(m, C + m, X + m, HSV.A); }
    else if (180 <= HSV.H && HSV.H < 240.0) { RGBA.All = V4(m, X + m, C + m, HSV.A); }
    else if (240 <= HSV.H && HSV.H < 300.0) { RGBA.All = V4(X + m, m, C + m, HSV.A); }
    else if (300 <= HSV.H && HSV.H < 360.0) { RGBA.All = V4(C + m, m, X + m, HSV.A); }

    return RGBA;
};

internal hsva_color
RGBtoHSV(rgba_color RGBA)
{
    float Cmax = Max(Max(RGBA.R, RGBA.G), RGBA.B);
    float Cmin = Min(Min(RGBA.R, RGBA.G), RGBA.B);
    float Delta = Cmax - Cmin;

    hsva_color HSVA = {};
    if (Delta < 0.0001f) { HSVA.H = 0.0f; }
    else if (Cmax == RGBA.R) { HSVA.H = 60.0f * FloatMod((RGBA.G - RGBA.B) / Delta, 6.0f); }
    else if (Cmax == RGBA.G) { HSVA.H = 60.0f * ((RGBA.B - RGBA.R) / Delta + 2.0f); }
    else if (Cmax == RGBA.B) { HSVA.H = 60.0f * ((RGBA.R - RGBA.G) / Delta + 4.0f); }

    if (Cmax < 0.0001f) { HSVA.S = 0.0f; }
    else { HSVA.S = Delta / Cmax; }

    HSVA.V = Cmax;
    HSVA.A = RGBA.A;
    return HSVA;
}

inline pixel_color
TO_U8_COLOR(hsva_color HSV)
{
    return TO_U8_COLOR(HSVtoRGB(HSV).All);
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
