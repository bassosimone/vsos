// File: include/errno.h
// Purpose: stripped down errno.h header
// SPDX-License-Identifier: MIT
#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <sys/cdefs.h> // for __BEGIN_DECLS
#include <sys/errno.h> // pull Exx definitions

__BEGIN_DECLS

extern volatile int errno;

__END_DECLS

#endif // __ERRNO_H__
