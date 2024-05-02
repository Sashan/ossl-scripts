/*
 * Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <string.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include "base64.h"

int
base64Encode(const char *bufIn, size_t bufInSz, char *bufOut, size_t bufOutSz)
{
	BIO	*base64 = NULL;
	BIO	*mem = NULL;
	char	scratch[20];
	int	rv = -1;

	base64 = BIO_new(BIO_f_base64());
	mem = BIO_new(BIO_s_mem());
	if ((base64 == NULL) || (mem == NULL))
		goto error;

	BIO_push(base64, mem);

	rv = BIO_write(base64, bufIn, (int)bufInSz);
	if ((rv < 0) || (rv < (int)bufInSz))
		goto error;
	BIO_flush(base64);

	rv = BIO_read(mem, bufOut, bufOutSz);
	if (BIO_read(mem, scratch, sizeof (scratch)) > 0)
	   rv = -1;
error:
	BIO_free(base64);
	BIO_free(mem);
	return (rv);
}

int
base64Decode(const char *bufIn, size_t bufInSz, char *bufOut, size_t bufOutSz)
{
	BIO	*base64 = NULL;
	BIO	*mem = NULL;
	char	scratch[20];
	int	rv = -1;

	base64 = BIO_new(BIO_f_base64());
	mem = BIO_new_mem_buf(bufIn, bufInSz);
	if ((base64 == NULL) || (mem == NULL))
		goto error;

	BIO_push(base64, mem);

	rv = BIO_read(base64, bufOut, bufOutSz);
	if (BIO_read(base64, scratch, sizeof (scratch)) > 0)
	   rv = -1;
error:
	BIO_free(mem);
	BIO_free(base64);
	return (rv);
}

static void
fakeCleanup(void)
{
	printf("%s done\n", __func__);
}

void
base64Init(void)
{
	(void)OPENSSL_atexit(fakeCleanup);
}
