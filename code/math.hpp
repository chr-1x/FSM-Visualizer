/* math.hpp 
 * By Andrew Chronister, (c) 2016
 *
 * Vector math related structures and computation functions.  Note that this is
 * a subset of a larger library of linear algebra-related functions, so there
 * are likely a number of functions defined here that are not used anywhere in
 * particular. A decent compiler should optimize them out.
 *
 * This file is defined as an .hpp file instead of a .h/.cpp pair so that the
 * math functions may be inlined at the compiler's discretion.
 */

#pragma once

// Purpose: Convenience typedefs and macro defintions
#include "types.h"

// Purpose: Math intrinsics, hopefully inlined as CPU instructions in most cases
// by the optimizer
#include <cmath>

/// Int vectors/structures

// Forward declarations of types to which some of the types below may be cast.
union u8x3;
union u8x4;
union vec2;

/* A two-dimensional vector with integral components.
 * Useful for points or specifying dimensions of discrete objects such as
 * bitmaps. */
union ivec2 {
    struct {
        int X, Y;
    };
    struct {
        int W, H;
    };
    struct {
        int Width, Height;
    };
    int E[2];

    operator vec2();
};

/* A 32-bit structure carefully defined to allow 4-channel 8-bit color to be
 * conveniently accessed with a typecast.
 * The anonymous structure is for RGBA byte order, a la PNG, OpenGL, etc.
 * The 'to' structure is for BGRA byte order, a la Windows DIBs.
 */
#pragma pack(push, 1)
union u8x4 {
    struct {
        u8 R;
        u8 G;
        u8 B;
        u8 A;
    };
    struct
    {
        u8 B;
        u8 G;
        u8 R;
        u8 A;
    } to;
    u8 E[4];
    u32 Color;
};
#pragma pack(pop)

/* Alternate name for the above structure. */
typedef u8x4 pixel_color;

/// Float vectors/structures

/* Two dimensional vector with floating-point components.
 * Good for representing values in a subset of the real-valued plane */
union vec2
{
	struct { f32 x, y; };
	struct { f32 X, Y; };
	struct { f32 u, v; };
	f32 E[2];
};

union vec3
{
	struct { f32 x, y, z; };
	struct { f32 X, Y, Z; };
	struct { f32 u, v, w; };
	struct { f32 r, g, b; };
	struct
	{
		vec2 xy;
		f32 _Ignored0;
	};	
	struct
	{
		vec2 yz;
		f32 _Ignored1;
	};	
	struct
	{
		vec2 uv;
		f32 _Ignored2;
	};	
	struct
	{
		f32 _Ignored3;
		vec2 vw;
	};

	f32 E[3];
};

union vec4
{
	struct { f32 x, y, z, w; };
	struct { f32 X, Y, Z, W; };
	struct { f32 r, g, b, a; };
	struct
	{
		vec2 xy;
		vec2 zw;
	};
	struct
	{
		f32 _Ignored0;
		vec2 yz;
		f32 _Ignored1;
	};
	struct
	{
		vec3 rgb;
		f32 _Ignored2;
	};	
	struct
	{
		vec3 xyz;
		f32 _Ignored3;
	};
	f32 E[4];
};

//
//NOTE(chronister): Integer operations
//

inline int
Clamp0_255(int A) {
    return Max(Min(A, 255), 0);
}

//
// NOTE(chronister): Scalar operations
//

#define PI 3.141592653589793238462643383279502
#define PI32 (float32)PI
#define SQRT2 1.4142135623730950488016887242097
#define SQRT3 1.7320508075688772935274463415059
#define INV_SQRT2 0.70710678118654752440084436210485

inline f32
Square(f32 A)
{
	f32 Result = A*A;
	return Result;
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
	f32 Result = (1.0f - t)*A + t*B;

	return Result;
}

internal f32
Unlerp(f32 RangeMin, f32 t, f32 RangeMax)
{
    return (t - RangeMin) / (RangeMax - RangeMin);
}

internal f32
RemapRange(f32 Val, f32 SourceMin, f32 SourceMax, f32 DestMin, f32 DestMax)
{
    return Lerp(DestMin, Unlerp(SourceMin, Val, SourceMax), DestMax);
}

internal f32
ModRange(f32 Val, f32 Max)
{
    while (Val < 0.0f) { Val += Max; }
    while (Val >= Max) { Val -= Max; }
    return Val;
}

inline f32
Clamp(f32 Min, f32 Value, f32 Max)
{
	f32 Result = Value;
	if (Result < Min) { Result = Min; }
	if (Result > Max) { Result = Max; }
	return Result;
}

inline f32
Clamp01(f32 Value)
{
	return Clamp(0.0f, Value, 1.0f);
}

inline f32
Clamp01MapToRange(f32 Min, f32 t, f32 Max)
{
	f32 Result = 0.0f;

	f32 Range = Max - Min;
	if (Range != 0)
	{
		Result = Clamp01((t - Min) / Range);
	}

	return Result;
}

#define SafeRatio0(Num, Div) SafeRatioN(Num, Div, 0.0f)
#define SafeRatio1(Num, Div) SafeRatioN(Num, Div, 1.0f)
inline f32
SafeRatioN(f32 Numerator, f32 Divisor, f32 N)
{
	f32 Result = N;
	if (Divisor != 0.0f)
		Result = Numerator / Divisor;
	return Result;
}

inline f32
SafeInverse(f32 Num)
{
	return SafeRatio0(1.0f, Num);
}

//
// NOTE(chronister): ivec2 operations
//

inline ivec2
IV2(int X, int Y) {
    ivec2 Result = { X, Y };
    return Result;
}

inline 
ivec2::operator vec2()
{
    vec2 Result = { (f32)this->X, (f32)this->Y };
    return Result;
}

inline ivec2 
operator*(float C, ivec2 A)
{
    ivec2 Result;
    Result.X = (int)(A.X * C);
    Result.Y = (int)(A.Y * C);
    return Result;
}

inline int
Inner(ivec2 A, ivec2 B)
{
    int Result = A.X * B.X + A.Y * B.Y;
    return Result;
}

inline ivec2
Hadamard(ivec2 A, ivec2 B)
{
    ivec2 Result = {};
    Result.X = A.X * B.X;
    Result.Y = A.Y * B.Y;
    return Result;
}

inline ivec2 
operator*(ivec2 A, float C) { return C * A; }

inline ivec2 
operator/(ivec2 A, float C) { return (1.0f/C) * A; }

inline ivec2
operator+(ivec2 A, ivec2 B)
{ 
    ivec2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}

inline ivec2
operator-(ivec2 A)
{
    ivec2 Result = { -A.X, -A.Y };
    return Result;
}

inline ivec2
operator-(ivec2 A, ivec2 B)
{
    ivec2 Result = { A.X - B.X, A.Y - B.Y };
    return Result;
}

inline vec2
operator+(vec2 A, ivec2 B)
{
    vec2 Result = { A.x + B.X, A.y + B.Y };
    return Result;
}

//
// NOTE(chronister): vec2 operations
//

inline vec2
V2(int32 x, int32 y)
{
	vec2 Result = {(f32)x, (f32)y};
	return Result;
}

inline vec2
V2u(u32 x, u32 y)
{

	vec2 Result = {(f32)x, (f32)y};
	return Result;
}

inline vec2 
V2(f32 x, f32 y)
{
	vec2 Result;

	Result.x = x;
	Result.y = y;

	return Result;
}

inline vec2
operator*(f32 A, vec2 B)
{
	vec2 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;

	return Result;
}

inline vec2
operator*(vec2 B, f32 A)
{
	vec2 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;

	return Result;
}

inline vec2
operator/(vec2 B, f32 A) { return B * (1.0f / A); }

inline vec2 &
operator*=(vec2 &A, f32 B)
{
	A = B * A;
	return A;
}

inline vec2 
operator-(vec2 A)
{
	vec2 Result;

	Result.x = -A.x;
	Result.y = -A.y;

	return Result;
}

inline vec2 
operator+(vec2 A, vec2 B)
{
	vec2 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;

	return Result;
}

inline vec2 &
operator+=(vec2 &A, vec2 B)
{
	A = B + A;
	return(A);
}

inline vec2 
operator-(vec2 A, vec2 B)
{
	vec2 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;

	return Result;
}

inline vec2
Hadamard(vec2 A, vec2 B)
{
	vec2 Result;
	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	return Result;
}

inline f32
Inner(vec2 A, vec2 B)
{
	f32 Result = A.x * B.x + A.y * B.y;
	return Result;
}

inline vec2
Perp(vec2 A)
{
	vec2 Result = { -A.y, A.x };
	return Result;
}

inline vec2
SwapComponents(vec2 A)
{
	vec2 Result = { A.y, A.x };
	return Result;
}

inline f32
LengthSq(vec2 A)
{
	f32 Result = Inner(A, A);

	return Result;
}

inline f32
Length(vec2 A)
{
	f32 Result = sqrtf(LengthSq(A));
	return Result;
}

inline vec2
Normalize(vec2 A)
{
	vec2 Result = A * SafeRatio0(1.0f, Length(A));
	return Result;
}

inline vec2
Clamp(vec2 Min, vec2 Value, vec2 Max)
{
	vec2 Result;
	Result.x = Clamp(Min.x, Value.x, Max.x);
	Result.y = Clamp(Min.y, Value.y, Max.y);
	return Result;
}

inline vec2 
Clamp01(vec2 Value)
{
	return Clamp(V2(0.0f, 0.0f), Value, V2(1.0f, 1.0f));
}

inline vec2
SafeInverse(vec2 Value)
{
	vec2 Result;
	Result.x = SafeInverse(Value.x);
	Result.y = SafeInverse(Value.y);
	return Result;
}

inline vec2
LeftNormal(vec2 Value)
{
    return Normalize(Perp(Value));
}

inline vec2
RightNormal(vec2 Value)
{
    return Normalize(-Perp(Value));
}


//
// NOTE(chronister): vec3 operations
//

inline vec3 
V3(f32 x, f32 y, f32 z)
{
	vec3 Result;

	Result.x = x;
	Result.y = y;
	Result.z = z;

	return Result;
}

inline vec3 
V3(vec2 xy, f32 z)
{
	vec3 Result;

	Result.xy = xy;
	Result.z = z;

	return Result;
}

inline vec3 
V3(f32 x, vec2 yz)
{
	vec3 Result;

	Result.x = x;
	Result.yz = yz;

	return Result;
}

inline vec3
operator*(f32 A, vec3 B)
{
	vec3 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;
	Result.z = B.z * A;

	return Result;
}

inline vec3
operator*(vec3 B, f32 A)
{
	vec3 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;
	Result.z = B.z * A;

	return Result;
}

inline vec3 &
operator*=(vec3 &A, f32 B)
{
	A = B * A;
	return A;
}

inline vec3 
operator-(vec3 A)
{
	vec3 Result;

	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;

	return Result;
}

inline vec3 
operator+(vec3 A, vec3 B)
{
	vec3 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;

	return Result;
}

inline vec3 &
operator+=(vec3 &A, vec3 B)
{
	A = B + A;
	return(A);
}

inline vec3 
operator-(vec3 A, vec3 B)
{
	vec3 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;

	return Result;
}

inline vec3
Hadamard(vec3 A, vec3 B)
{
	vec3 Result;
	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;
	return Result;
}

inline vec3
InverseHadamard(vec3 A, vec3 B)
{
	vec3 Result;
	Result.x = A.x / B.x;
	Result.y = A.y / B.y;
	Result.z = A.z / B.z;
	return Result;
}

inline f32
Inner(vec3 A, vec3 B)
{
	f32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
	return Result;
}

inline vec3
Cross(vec3 U, vec3 V)
{
	vec3 Result = {
        U.Y * V.Z - U.Z * V.Y,
        U.Z * V.X - U.X * V.Z,
        U.X * V.Y - U.Y * V.X
    };
	return Result;
}

inline f32
LengthSq(vec3 A)
{
	f32 Result = Inner(A, A);

	return Result;
}

inline f32
Length(vec3 A)
{
	f32 Result = sqrtf(LengthSq(A));
	return Result;
}

inline vec3
Normalize(vec3 A)
{
	vec3 Result = A * (1.0f / Length(A));
	return Result;
}

inline vec3
Clamp(vec3 Min, vec3 Value, vec3 Max)
{
	vec3 Result;
	Result.x = Clamp(Min.x, Value.x, Max.x);
	Result.y = Clamp(Min.y, Value.y, Max.y);
	Result.z = Clamp(Min.z, Value.z, Max.z);
	return Result;
}

inline vec3
Clamp01(vec3 Value)
{
	return Clamp(V3(0.0f, 0.0f, 0.0f), Value, V3(1.0f, 1.0f, 1.0f));
}

inline vec3 
Lerp(vec3 A, f32 t, vec3 B)
{
	vec3 Result = (1.0f - t)*A + t*B;

	return Result;
}

//
// NOTE(chronister): ivec4 operations
//

inline u8x4 
HEX_RGB(u32 HexColor) {
    u8x4 To = {};
    To.R = (HexColor >> 16) & 0xFF;
    To.G = (HexColor >> 8) & 0xFF;
    To.B = (HexColor >> 0) & 0xFF;
    To.A = (HexColor >> 24) & 0xFF;
    return To;
}

inline u8x4
TO_U8_COLOR(vec4 Color)
{
    u8x4 Result = {};
    Result.R = (u8)(Color.r * 255);
    Result.G = (u8)(Color.g * 255);
    Result.B = (u8)(Color.b * 255);
    Result.A = (u8)(Color.a * 255);
    return Result;
}

inline u8x4
operator+(u8x4 A, u8x4 B) {
    u8x4 Result = {}; 
    Result.R = A.R + B.R;
    Result.G = A.G + B.G;
    Result.B = A.B + B.B;
    Result.A = A.A + B.A;
    return Result;
}

inline u8x4
operator*(u8x4 A, f32 C) {
    u8x4 Result = {}; 
    Result.R = (u8)(A.R * C);
    Result.G = (u8)(A.G * C);
    Result.B = (u8)(A.B * C);
    Result.A = (u8)(A.A * C);
    return Result;
}

//
// NOTE(chronister): vec4 operations
//

inline vec4 
V4(f32 x, f32 y, f32 z, f32 w)
{
	vec4 Result;

	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;

	return Result;
}

inline vec4 
V4(vec3 xyz, f32 w)
{
	vec4 Result;

	Result.xyz = xyz;
	Result.w = w;

	return Result;
}

inline vec4
operator*(f32 A, vec4 B)
{
	vec4 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;
	Result.z = B.z * A;
	Result.w = B.w * A;

	return Result;
}

inline vec4
operator*(vec4 B, f32 A)
{
	vec4 Result;

	Result.x = B.x * A;
	Result.y = B.y * A;
	Result.z = B.z * A;
	Result.w = B.w * A;

	return Result;
}

inline vec4 &
operator*=(vec4 &A, f32 B)
{
	A = B * A;
	return A;
}

inline vec4 
operator-(vec4 A)
{
	vec4 Result;

	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	Result.w = -A.w;

	return Result;
}

inline vec4 
operator+(vec4 A, vec4 B)
{
	vec4 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;

	return Result;
}

inline vec4 &
operator+=(vec4 &A, vec4 B)
{
	A = B + A;
	return(A);
}

inline vec4 
operator-(vec4 A, vec4 B)
{
	vec4 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;

	return Result;
}

inline vec4
Hadamard(vec4 A, vec4 B)
{
	vec4 Result;
	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;
	Result.w = A.w * B.w;
	return Result;
}

inline f32
Inner(vec4 A, vec4 B)
{
	f32 Result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
	return Result;
}

inline f32
LengthSq(vec4 A)
{
	f32 Result = Inner(A, A);

	return Result;
}

inline f32
Length(vec4 A)
{
	f32 Result = sqrtf(LengthSq(A));
	return Result;
}

inline vec4 
Lerp(vec4 A, f32 t, vec4 B)
{
	vec4 Result = (1.0f - t)*A + t*B;

	return Result;
}

