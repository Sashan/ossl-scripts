#
# Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html
#

#
# Just in case you want to byild with RPATH option:
#-Wl,-rpath,$(TARGET_OSSL_LIBRARY_PATH)
#

all: test.implicit test.explicit base64.so base64-static.so

LDFLAGS=-L $(TARGET_OSSL_LIBRARY_PATH)
CPPFLAGS=-I$(TARGET_OSSL_INCLUDE_PATH)
#
# _GNI_SOURCE guard is needed to include strcasecmp()
# on linuc/glibc
#
CFLAGS=-g -O0 -D_GNU_SOURCE

test.implicit: implicit.o base64.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -o test.implicit implicit.o base64.o \
	    $(LDFLAGS) -lcrypto

implicit.o: implicit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

base64.o: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

test.explicit: explicit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o test.explicit explicit.c

base64.so: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -fPIC -o base64.so base64.c \
	    $(LDFLAGS) -lcrypto

base64-static.so: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -fPIC \
	    -o base64-static.so base64.c \
	    $(TARGET_OSSL_LIBRARY_PATH)/libcrypto.a -lpthread

test: test-atexit-implicit test-atexit-explicit test-atexit-explicit-static \
	test-cleanup-implicit test-cleanup-explicit test-cleanup-explicit-static

test-atexit-implicit: test.implicit
	./test.implicit |grep 'base64AtExit'

test-atexit-explicit: test.explicit base64.so
	./test.explicit -l `pwd`/base64.so -f 'now|global' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64.so -f 'now|local' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64.so -f 'lazy|global' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64.so -f 'lazy|local' | grep 'base64AtExit'

test-atexit-explicit-static: test.explicit base64-static.so
	./test.explicit -l `pwd`/base64-static.so -f 'now|global' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64-static.so -f 'now|local' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|global' | grep 'base64AtExit'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|local' | grep 'base64AtExit'

test-cleanup-implicit: test.implicit
	./test.implicit |grep 'OPENSSL_cleanup'

test-cleanup-explicit: test.explicit base64.so
	./test.explicit -l `pwd`/base64.so -f 'now|global' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64.so -f 'now|local' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64.so -f 'lazy|global' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64.so -f 'lazy|local' |grep 'OPENSSL_cleanup'

test-cleanup-explicit-static: test.explicit base64.so
	./test.explicit -l `pwd`/base64-static.so -f 'now|global' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64-static.so -f 'now|local' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|global' |grep 'OPENSSL_cleanup'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|local' |grep 'OPENSSL_cleanup'

clean:
	rm -f test.implicit base64.o base64.so base64-static.so test.explicit
