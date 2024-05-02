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
typedef void(*Base64Init)(void);

static Base64Func base64Decode;
static Base64Func base64Encode;
static Base64Init base64Init;

static const char *progName = "???";

static void
usage(void)
{
	fprintf(stderr,
	    "%s - unknown option, run program use option as follows\n"
	    "\t%s -l path_to_lib -f [{now,lazy}|{global,local}\n",
	    progName, progName);
	exit(1);
}

static void
runTest(void)
{
	char	buf1[1024], buf2[1024];
	char	*msg = "Hello World, base64 is linked implicitly\n";
	int	len;

	/*
	 * ask base64 to install exit handler via OPENSSL_atexit();
	 * I things go as expected 'fakeCleanup done' appears on stdout.
	 */
	base64Init();

	if ((len = base64Encode(msg, strlen(msg), buf1, sizeof (buf1))) == -1)
		printf("base64Encode() has failed\n");

	if (base64Decode(buf1, (size_t)len, buf2, sizeof (buf2)) == -1)
		printf("base64Decode() has failed\n");

	printf("%s\n", buf2);
}

typedef struct LinkerFlag {
	const char *flagName;
	int flag;
} LinkerFlagT;

static int
nameToFlag(const char *flag)
{
	static LinkerFlagT table[] = {
		{ "now",	RTLD_NOW },
		{ "lazy",	RTLD_LAZY },
		{ "global",	RTLD_GLOBAL },
		{ "local",	RTLD_LOCAL },
		{ NULL, -1 }
	};
	LinkerFlagT *t;

	t = &table[0];
	while ((t->flagName != NULL) &&
	    (strcasestr(t->flagName, flag) == NULL))
		t++;

	if (t->flagName == NULL)
		err(1, "Invalid flag %s\n", flag);

	return (t->flag);
}

int
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

int
main(int argc, char * const *argv)
{
	int	c;
	const char *libName, *flags;
	void	*dl;


	progName = argv[0];

	while ((c = getopt(argc, argv, "l:f:")) != -1) {
		switch (c) {
		case 'l':
			libName = optarg;
			break;
		case 'f':
			flags = optarg;
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

	base64Init = dlsym(dl, "base64Init");
	if (base64Init == NULL)
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
