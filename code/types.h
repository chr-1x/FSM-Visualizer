/* types.h
 * by Andrew Chronister, (c) 2016
 *
 * A number of typedefs and preprocessor macros for convenience use around the
 * project. */
#pragma once

// Purpose: standardized type sizes which eliminate the guesswork from C's
// short/int/long nonsense.
#include <cstdint>

// Purpose: Standard common functions, EXIT_FAILURE and EXIT_SUCCESS macros,
// size_t for platform-sized unsigned integer, and NULL for null pointer
#include <cstdlib>

/* Shorthand for Signed integer types of various sizes */
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/* Shorthand for Unsigned integer types of various sizes */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Generic unsigned integer shorthand. Good if unsignedness is needed but a
 * particular size has not yet been decided. */
typedef unsigned int uint;

/* Shorthand for Floating-point types of small and large sizes */
typedef float f32;
typedef double f64;

/* Some disambiguation macros that clarify the intended meaning of the "static"
 * keyword in various contexts. */

/* Static as-in global variable whose storage is in the data section of the
 * program */
#define global_variable static
/* Static as-in a function definition that shouldn't be exposed from the current
 * module. */
#define internal static
/* Static as-in a variable whose value persists between function calls.
 * Technically the same as the global_variable usage, but semantically different
 * due to scoping rules. Mostly useful for grepping to remove instances of
 * usage. */
#define local_persist static

/* Utility macros for common simple operations on generic data types.
 * Much easier to define and use than template counterparts. */

/* Gives the length of a statically-allocated array. Requires that the compiler
 * know the length of the array at compile-time. */
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
/* Subtitutes in a statement which should evaluate to the lesser of two values.
 * WARNING: Evaluates at least one of its arguments twice! */
#define Min(A, B) (((A) < (B)) ? (A) : (B))
/* Subtitutes in a statement which should evaluate to the greater of two values.
 * WARNING: Evaluates at least one of its arguments twice! */
#define Max(A, B) (((A) > (B)) ? (A) : (B))

/* Macros for expressing byte quantities more intuitively. */
#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

