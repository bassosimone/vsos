// File: include/sys/param.h
// Purpose: architecture agnostic parameters.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef __SYS_PARAM_H__
#define __SYS_PARAM_H__

// Frequency at which the clock scheduler ticks.
#define HZ 100

// Page size on the amd64 architecture.
#define MM_PAGE_SIZE 4096UL

// Alias for MM_PAGE_SIZE.
#define PAGE_SIZE MM_PAGE_SIZE

// Maximum number of runnable threads.
#define SCHED_MAX_THREADS 32

// Alias for SCHED_MAX_THREADS.
#define MAX_THREADS SCHED_MAX_THREADS

#endif // __SYS_PARAM_H__
