// File: shell/shell.c
// Purpose: Very simple user shell
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <libc/string/string.h> // for strncmp
#include <libc/unistd/unistd.h> // for write

__attribute__((section(".text.start"))) [[noreturn]] void _start(void) {
	while (1) {
	prompt:
		(void)write(1, "> ", 2);
		char cmdline[128];
		for (int idx = 0;; idx++) {
			ssize_t rc = read(0, &cmdline[idx], 1);
			if (rc != 1) {
				goto prompt;
			}
			(void)write(1, &cmdline[idx], 1);
			if (idx == sizeof(cmdline) - 1) {
				(void)write(1, "\nERR\n", 5);
				goto prompt;
			}
			if (cmdline[idx] == '\r') {
				(void)write(1, "\n", 1);
				cmdline[idx] = '\0';
				break;
			}
		}

		if (strncmp(cmdline, "hello", 5) == 0) {
			(void)write(1, "HELLO\n", 6);
			continue;
		}
		(void)write(1, "U\n", 2);
	}
}
