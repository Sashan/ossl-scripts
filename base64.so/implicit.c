/*
 * Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <string.h>

#include "base64.h"

#define	BUFSZ	1024

int
main(int argc, const char *argv[])
{
	char	buf1[BUFSZ], buf2[BUFSZ];
	char	*msg = "Hello World, base64 is linked implicitly\n";
	int	len;

	base64ArmAtExit();

	memset(buf1, 0, BUFSZ);
	memset(buf2, 0, BUFSZ);

	if ((len = base64Encode(msg, strlen(msg), buf1, sizeof (buf1))) == -1)
		printf("base64Encode() has failed\n");

	if (base64Decode(buf1, (size_t)len, buf2, sizeof (buf2)) == -1)
		printf("base64Decode() has failed\n");

	printf("%s\n", buf2);

	return (0);
}
