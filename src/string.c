/*
 * These are the public domain implementations of the stpecpy and
 * strtcpy functions taken from the string_copying(7) man page.
 *
 * They are safer alternatives to strncat and strncpy that actually
 * operate on strings rather than null-padded character arrays and
 * ensure that the resulting string is always null-terminated.
 *
 * See string_copying(7) for documentation.
 */

/* This code is in the public domain.  */

#include "hack.h"

#include <string.h>
#include <errno.h>

char *
stpecpy(char *dst, char end[0], const char *restrict src)
{
	size_t dlen;

	if (dst == NULL)
		return NULL;

	dlen = strtcpy(dst, src, end - dst);
	return (dlen == -1) ? NULL : dst + dlen;
}

ssize_t
strtcpy(char *restrict dst, const char *restrict src, size_t dsize)
{
	boolean	trunc;
	size_t	dlen, slen;

	if (dsize == 0) {
		errno = ENOBUFS;
		return -1;
	}

	slen = strnlen(src, dsize);
	trunc = (slen == dsize);
	dlen = slen - trunc;

	stpcpy(mempcpy(dst, src, dlen), "");
	if (trunc)
		errno = E2BIG;
	return trunc ? -1 : slen;
}
