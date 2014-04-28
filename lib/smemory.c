/**
 * @file   smemory.c
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Tue May 22 00:41:24 2012
 * 
 * @brief  Small Memory Wrapper
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "internal.h"

LWQQ_EXPORT
void *s_malloc(size_t size)
{
	return size?malloc(size):NULL;
}

LWQQ_EXPORT
void *s_malloc0(size_t size)
{
	if (size == 0)
		return NULL;

	void *ptr = malloc(size);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

LWQQ_EXPORT
void *s_calloc(size_t nmemb, size_t lsize)
{
	return calloc(nmemb, lsize);
}

LWQQ_EXPORT
void *s_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

LWQQ_EXPORT
char *s_strdup(const char *s1)
{
	return s1?strdup(s1):NULL;
}

LWQQ_EXPORT
long s_atol(const char* s,long init)
{
	char* end;
	if(!s) return init;
	long ret = strtol(s,&end,10);
	return (end==s)?init:ret;
}
#if 0
LWQQ_EXPORT
char *s_strndup(const char *s1, size_t n)
{
	return s1?strndup(s1, n):NULL;
}

LWQQ_EXPORT
int s_vasprintf(char **buf, const char * format,
		va_list arg)
{
	return vasprintf(buf, format, arg);
}

LWQQ_EXPORT
int s_asprintf(char **buf, const char *format, ...)
{
	va_list arg;
	int rv;

	va_start(arg, format);
	rv = s_vasprintf(buf, format, arg);
	va_end(arg);

	return rv;
}
#endif 

// vim: ts=3 sw=3 sts=3 noet
