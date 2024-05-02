#
# Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html
#

all: test.implicit test.explicit base64.so base64-static.so

LDFLAGS=-L $(TARGET_OSSL_LIBRARY_PATH)
CPPFLAGS=-I$(TARGET_OSSL_INCLUDE_PATH)
CFLAGS=-g -O0

test.implicit: implicit.o base64.o
	$(CC) $(LDFLAGS) -lcrypto -o test.implicit implicit.o base64.o 

implicit.o: implicit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

base64.o: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

test.explicit: explicit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o test.explicit explicit.c

base64.so: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -fPIC $(LDFLAGS) -lcrypto \
	    -o base64.so base64.c

base64-static.so: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -FPIC \
	    $(TARGET_OSSL_LIBRARY_PATH)/libcrypto.a -lpthread \
	    -o base64-static.so base64.c

test: test-implicit test-explicit test-explicit-static

test-implicit: test.implicit
	./test.implicit |grep 'fakeCleanup done'

test-explicit: test.explicit base64.so
	./test.explicit -l `pwd`/base64.so -f 'now|global' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64.so -f 'now|local' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64.so -f 'lazy|global' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64.so -f 'lazy|local' | grep 'fakeCleanup done'

test-explicit-static: test.explicit base64-static.so
	./test.explicit -l `pwd`/base64-static.so -f 'now|global' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64-static.so -f 'now|local' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|global' | grep 'fakeCleanup done'
	./test.explicit -l `pwd`/base64-static.so -f 'lazy|local' | grep 'fakeCleanup done'

clean:
	rm -f test.implicit base64.o base64.so base64-static.so test.explicit
