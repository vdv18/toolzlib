#ifndef M_LOG_H
#define M_LOG_H

#include <stdio.h>
#include <stdarg.h>

#define L_ERROR 1
#define L_WARN 2
#define L_DEBUG12 12
#define LOG(args...) _M_LOG(args)

inline void _M_LOG(int level, const char *format, ...){
    va_list va;
    va_start(va, format);
    int ret = vfprintf(stderr, format, va);
    va_end(va);
}

#endif