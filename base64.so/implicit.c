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
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#include "base64.h"

#define	MAIN_AT_EXIT	1
#define	BASE64_AT_EXIT	2

#define	BUFSZ	1024

typedef struct TableEntry {
	const char *flagName;
	int flag;
} TableEntryT;

static void
usage(const char *progName)
{
	fprintf(stderr,
	    "%s - unknown option, run program use option as follows\n"
	    "\t-a [base64AtExit,mainAtExit} which at exit handler should be set.\n",
	    progName);
	exit(1);
}

static int
wordToFlag(TableEntryT *t, const char *word)
{
	while ((t->flagName != NULL) &&
	    (strcasestr(t->flagName, word) == NULL))
		t++;

	return (t->flag);
}

static int
nameToExitFunc(const char *atExit)
{
	static TableEntryT table[] = {
		{"mainAtExit",		MAIN_AT_EXIT },
		{"base64AtExit",	BASE64_AT_EXIT },
		{ NULL, -1 }
	};
	return (wordToFlag(table, atExit));
}

static int
parseExitHandler(const char *atExit)
{
	char *a = strdup(atExit);
	char *sep, *atExitFunc;
	int func;
	int rv = 0;

	if (a == NULL)
		errx(1, "memory");

	while ((sep = strchr(a, ',')) != NULL) {
		*sep = '\0';
		func = nameToExitFunc(atExit);
		if (func == -1)
			err(1, "unknown atExit handler func: %s\n", atExitFunc);
		rv |= func;
	}

	func = nameToExitFunc(atExit);
	if (func == -1)
		err(1, "unknown atExit handler func: %s\n", atExit);

	rv |= func;
	free(a);

	return (rv);
}

static void
mainAtExit(void)
{
	printf("%s\n", __func__);
}


int
main(int argc, char * const *argv)
{
	char	buf1[BUFSZ], buf2[BUFSZ];
	char	*atExit;
	char	*msg = "Hello World, base64 is linked implicitly\n";
	int	len, atExitFuncs;
	int 	c;

	while ((c = getopt(argc, argv, "a:")) != -1) {
		switch (c) {
		case 'a':
			atExit = optarg;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (atExit != NULL)
		atExitFuncs = parseExitHandler(atExit);

	memset(buf1, 0, BUFSZ);
	memset(buf2, 0, BUFSZ);

	if ((atExitFuncs & MAIN_AT_EXIT) == MAIN_AT_EXIT)
		base64ArmAtExit(mainAtExit);

	if ((atExitFuncs & BASE64_AT_EXIT) == BASE64_AT_EXIT)
		base64ArmAtExit(NULL);

	if ((len = base64Encode(msg, strlen(msg), buf1, BUFSZ)) == -1)
		printf("base64Encode() has failed\n");

	if (base64Decode(buf1, (size_t)len, buf2, BUFSZ) == -1)
		printf("base64Decode() has failed\n");

	printf("%s\n", buf2);

	return (0);
}
