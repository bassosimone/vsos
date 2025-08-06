// File: kernel/core/printk.c
// Purpose: implement the printk function.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/printk.h>
#include <kernel/tty/uart.h>
#include <libc/string/string.h>

// Note: this code cannot invoke `KERNEL_ASSERT` since it is called by `panic`.

// We will consider moving this function elsewhere if needed. For now, it
// seems to be an implementation detail of printk.
static inline int16_t uart_putchar(uint8_t ch) {
	while (!uart_poll_write()) {
		// TODO(bassosimone): should we yield here?
	}
	int16_t rv = uart_try_write(ch);
	// Note: we cannot `KERNEL_ASSERT(rv == 0)` here since this function is called by panic
	return rv;
}

// Internal helper to print a single character
static inline void _putchar(char ch) { (void)uart_putchar(ch); }

static void _string(const char *s) {
	while (*s != '\0') {
		_putchar(*s++);
	}
}

static void _udecimal(uint64_t value) {
	uint64_t divisor = 1;
	while (value / divisor > 9) {
		divisor *= 10;
	}
	while (divisor > 0) {
		_putchar('0' + value / divisor);
		value %= divisor;
		divisor /= 10;
	}
}

static void _decimal(int64_t value) {
	uint64_t uvalue = 0;
	if (value < 0) {
		_putchar('-');
		uvalue = ((uint64_t)(-value + 1)) + 1;
	} else {
		uvalue = (uint64_t)value;
	}
	_udecimal(uvalue);
}

static void _uhex(uint64_t value) {
	bool should_print = false;
	for (int idx = 15; idx >= 0; idx--) {
		int nibble = (value >> (idx * 4)) & 0xf;
		should_print = should_print || nibble != 0;
		if (should_print) {
			_putchar("0123456789abcdef"[nibble]);
		}
	}
}

void printk(const char *fmt, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	vprintk(fmt, ap);
	__builtin_va_end(ap);
}

void vprintk(const char *fmt, __builtin_va_list ap) {
	for (;;) {
		// 1. handle the case where the string ends
		if (*fmt == '\0') {
			return;
		}

		// 2. if the character is not `%` just copy it in output
		if (*fmt != '%') {
			_putchar(*fmt);
			continue;
		}

		// 3. skip over the `%` and check the next character
		fmt++;

		// 4. handle the case where the string ends
		if (*fmt == '\0') {
			_putchar('%');
			return;
		}

		// 5. single character format string

		// 5.1. handle the case of a quoted '%d'
		if (*fmt == '%') {
			_putchar('%');
			fmt++;
			continue;
		}

		// 5.2. handle the case of '%s'
		if (*fmt == 's') {
			_string(__builtin_va_arg(ap, const char *));
			fmt++;
			continue;
		}

		// 5.3. handle the case of '%d'
		if (*fmt == 'd') {
			_decimal(__builtin_va_arg(ap, int));
			fmt++;
			continue;
		}

		// 5.4. handle the case of '%u'
		if (*fmt == 'u') {
			_udecimal(__builtin_va_arg(ap, unsigned int));
			fmt++;
			continue;
		}

		// 5.5. handle the case of '%x'
		if (*fmt == 'x') {
			_uhex(__builtin_va_arg(ap, unsigned int));
			fmt++;
			continue;
		}

		// 6. two-characters format string

		// 6.1. handle the case of '%ld'
		if (strncmp(fmt, "ld", 2) == 0) {
			_decimal(__builtin_va_arg(ap, long));
			fmt = fmt + 2;
			continue;
		}

		// 6.2. handle the case of '%lu'
		if (strncmp(fmt, "lu", 2) == 0) {
			_udecimal(__builtin_va_arg(ap, unsigned long));
			fmt = fmt + 2;
			continue;
		}

		// 6.3. handle the case of '%lx'
		if (strncmp(fmt, "lx", 2) == 0) {
			_uhex(__builtin_va_arg(ap, unsigned long));
			fmt = fmt + 2;
			continue;
		}

		// 7. three-characters format string

		// 7.1. handle the case of '%lld'
		if (strncmp(fmt, "lld", 3) == 0) {
			_decimal(__builtin_va_arg(ap, long long));
			fmt = fmt + 3;
			continue;
		}

		// 7.2. handle the case of '%llu'
		if (strncmp(fmt, "llu", 3) == 0) {
			_udecimal(__builtin_va_arg(ap, unsigned long long));
			fmt = fmt + 3;
			continue;
		}

		// 7.3. handle the case of '%llx'
		if (strncmp(fmt, "llx", 3) == 0) {
			_uhex(__builtin_va_arg(ap, unsigned long long));
			fmt = fmt + 3;
			continue;
		}

		// 8. handle the unknown case
		_putchar('%');
		_putchar(*fmt);
		fmt++;
	}
}
