// File: include/sys/types.h
// Purpose: Types for 64 bit architectures (we don't support 32 bit).
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

// Basic integer types on the amd64 architecture.
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

// Limit definitions for the basic integer types.
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255

#define SHRT_MIN (-32768)
#define SHRT_MAX 32767
#define USHRT_MAX 65535

#define INT_MIN (-INT_MAX - 1)
#define INT_MAX 2147483647
#define UINT_MAX 4294967295U

#define LLONG_MAX 9223372036854775807LL
#define LLONG_MIN (-LLONG_MAX - 1LL)
#define ULLONG_MAX 18446744073709551615ULL

#define INT8_MIN SCHAR_MIN
#define INT8_MAX SCHAR_MAX
#define UINT8_MAX UCHAR_MAX

#define INT16_MIN SHRT_MIN
#define INT16_MAX SHRT_MAX
#define UINT16_MAX USHRT_MAX

#define INT32_MIN INT_MIN
#define INT32_MAX INT_MAX
#define UINT32_MAX UINT_MAX

#define INT64_MIN LLONG_MIN
#define INT64_MAX LLONG_MAX
#define UINT64_MAX ULLONG_MAX

// Additional integer types.
typedef uint64_t size_t;
typedef uint64_t uintptr_t;
typedef int64_t ssize_t;
typedef int64_t intptr_t;

// Limits definitions for the additional integer types.
#define SIZE_MAX UINT64_MAX
#define UINTPTR_MAX UINT64_MAX
#define SSIZE_MAX INT64_MAX
#define INTPTR_MAX INT64_MAX

#endif // __SYS_TYPES_H__
