#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>

typedef int8_t int8;
typedef int8_t s8;
typedef int16_t int16;
typedef int16_t s16;
typedef int32_t int32;
typedef int32_t s32;
typedef int64_t int64;
typedef int64_t s64;

typedef uint8_t uint8;
typedef uint8_t u8;
typedef uint16_t uint16;
typedef uint16_t u16;
typedef uint32_t uint32;
typedef uint32_t u32;
typedef uint64_t uint64;
typedef uint64_t u64;

typedef unsigned int uint;

typedef float float32;
typedef float f32;
typedef double float64;
typedef double f64;

#define global_variable static
#define internal static
#define local_persist static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define Swap(A, B, T) { T temp##T = A; A = B; B = temp##T; }

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

