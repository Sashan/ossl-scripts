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
#include <dlfcn.h>
#include <err.h>

typedef int(*Base64Func)(const char *, size_t, char *, size_t);
typedef void(*Base64ArmAtExitFuc)(void(*)(void));

static Base64Func base64Decode;
static Base64Func base64Encode;
static Base64ArmAtExitFuc base64ArmAtExit;

static int atExitFuncs;
static const char *progName = "???";

#define	MAIN_AT_EXIT	1
#define	BASE64_AT_EXIT	2

#define	BUFSZ	1024

static void
usage(void)
{
	fprintf(stderr,
	    "%s - unknown option, run program use option as follows\n"
	    "\t%s -l path_to_lib -f [{now,lazy}|{global,local}\n"
	    "\t-a [base64AtExit,mainAtExit} which at exit handler should be set.\n",
	    progName, progName);
	exit(1);
}

static void
mainAtExit(void)
{
	printf("%s\n", __func__);
}

static void
runTest(void)
{
	char	buf1[BUFSZ], buf2[BUFSZ];
	char	*msg = "Hello World, base64 is linked implicitly\n";
	int	len;

	if ((atExitFuncs & MAIN_AT_EXIT) == MAIN_AT_EXIT)
		base64ArmAtExit(mainAtExit);

	if ((atExitFuncs & BASE64_AT_EXIT) == BASE64_AT_EXIT)
		base64ArmAtExit(NULL);

	memset(buf1, 0, BUFSZ);
	memset(buf2, 0, BUFSZ);

	if ((len = base64Encode(msg, strlen(msg), buf1, BUFSZ)) == -1)
		printf("base64Encode() has failed\n");

	if (base64Decode(buf1, (size_t)len, buf2, BUFSZ) == -1)
		printf("base64Decode() has failed\n");

	printf("%s\n", buf2);
}

typedef struct TableEntry {
	const char *flagName;
	int flag;
} TableEntryT;

static int
wordToFlag(TableEntryT *t, const char *word)
{
	while ((t->flagName != NULL) &&
	    (strcasestr(t->flagName, word) == NULL))
		t++;

	return (t->flag);
}

static int
nameToFlag(const char *flag)
{
	static TableEntryT table[] = {
		{ "now",	RTLD_NOW },
		{ "lazy",	RTLD_LAZY },
		{ "global",	RTLD_GLOBAL },
		{ "local",	RTLD_LOCAL },
		{ NULL, -1 }
	};
	int f;

	f = wordToFlag(table, flag);
	if (f  == -1)
		err(1, "Invalid flag %s\n", flag);

	return (f);
}

static int
parseFlags(const char *flags)
{
	char *lf = strdup(flags);
	char *sep, *flag;
	int  rv = 0;

	if (lf == NULL)
		errx(1, "memory");

	flag = lf;
	while ((sep = strchr(flag, '|')) != NULL) {
		*sep = '\0';
		rv |= nameToFlag(flag);
		*sep = '|';
		flag = &sep[1];
	}

	if ((rv == 0) && (*flag == '\0'))
		usage();

	rv |= nameToFlag(flag);

	if ((rv & (RTLD_NOW | RTLD_LAZY)) == (RTLD_NOW | RTLD_LAZY))
		err(1, "Flags 'now' and 'lazy' are mutually exclusive");

	free(lf);

	return (rv);
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
		err(1, "unknown atExit handler func: %s\n", atExitFunc);

	rv |= func;

	return (rv);
}

int
main(int argc, char * const *argv)
{
	int	c;
	const char *libName, *flags, *atExit;
	void	*dl;

	progName = argv[0];

	libName = NULL;
	flags = NULL;
	atExit = NULL;

	while ((c = getopt(argc, argv, "a:l:f:")) != -1) {
		switch (c) {
		case 'l':
			libName = optarg;
			break;
		case 'f':
			flags = optarg;
			break;
		case 'a':
			atExit = optarg;
			break;
		default:
			usage();
		}
	}

	if ((flags == NULL) || (libName == NULL))
		usage();

	dl = dlopen(libName, parseFlags(flags));
	if (dl == NULL)
		errx(1, "dlopen [%s]", dlerror());

	if (atExit != NULL)
		atExitFuncs = parseExitHandler(atExit);

	base64ArmAtExit = dlsym(dl, "base64ArmAtExit");
	if (base64ArmAtExit == NULL)
		errx(1, "dlsaym(\"base64Init\") [%s]", dlerror());

	base64Encode = dlsym(dl, "base64Encode");
	if (base64Encode == NULL)
		errx(1, "dlsaym(\"base64Encode\") [%s]", dlerror());

	base64Decode = dlsym(dl, "base64Decode");
	if (base64Decode == NULL)
		errx(1, "dlsaym(\"base64Decode\") [%s]", dlerror());

	runTest();

	dlclose(dl);

	return (0);
}
