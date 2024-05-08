/*
 * Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef	_BASE64_H_
#define	_BASE64_H_

#include <sys/param.h>

extern int base64Encode(const char *, size_t, char *, size_t);
extern int base64Decode(const char *, size_t, char *, size_t);
extern void base64ArmAtExit(void);

#endif
