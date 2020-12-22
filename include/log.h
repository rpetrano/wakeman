#ifndef _WAKEMAN_LOG_H
#define _WAKEMAN_LOG_H

#include <stdarg.h>
#include <string.h>
#include <errno.h>

enum log_importance {
	LOG_SILENT = 0,
	LOG_ERROR = 1,
	LOG_INFO = 2,
	LOG_DEBUG = 3,
	LOG_IMPORTANCE_LAST,
};

void wakeman_log_init(enum log_importance verbosity);

#ifdef __GNUC__
#define _ATTRIB_PRINTF(start, end) __attribute__((format(printf, start, end)))
#else
#define _ATTRIB_PRINTF(start, end)
#endif

void _wakeman_log(enum log_importance verbosity, const char *format, ...)
	_ATTRIB_PRINTF(2, 3);

const char *_wakeman_strip_path(const char *filepath);

#define wakeman_log(verb, fmt, ...) \
	_wakeman_log(verb, "[%s:%d] " fmt, _wakeman_strip_path(__FILE__), \
			__LINE__, ##__VA_ARGS__)

#define wakeman_log_errno(verb, fmt, ...) \
	wakeman_log(verb, fmt ": %s", ##__VA_ARGS__, strerror(errno))

#endif
