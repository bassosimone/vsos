// File: sys/cdefs.h
// Purpose: macros to transparently handle C++.
// SPDX-License-Identifier: MIT
#ifndef __SYS_CDEFS_H__
#define __SYS_CDEFS_H__

#ifdef __cplusplus

#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#define __NOEXCEPT noexcept

#else

#define __BEGIN_DECLS /* Nothing */
#define __END_DECLS   /* Nothing */
#define __NOEXCEPT    /* Nothing */

#endif // __cplusplus

#endif // __SYS_CDEFS_H__
