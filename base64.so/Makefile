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

all: test.static test.implicit test.explicit base64.so base64-static.so

LDFLAGS=-L $(TARGET_OSSL_LIBRARY_PATH)
CPPFLAGS=-I$(TARGET_OSSL_INCLUDE_PATH)
#
# _GNI_SOURCE guard is needed to include strcasecmp()
# on linuc/glibc
#
CFLAGS=-g -O0 -D_GNU_SOURCE

test.static: implicit.o base64.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -static -o test.static implicit.o base64.o \
	    -lpthread $(LDFLAGS) -lcrypto

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

#
# NOTE 1
# all exit handlers are set by `OPENSSL_atexit()`
# 
# NOTE 2
# the base64ArmAtExit() function used by test calls OPENSSL_init_crypto().
# Call to OPENSSL_init_crypto() is important in openssl-3.3 and earlier. Those
# versions require openssl to be initalized enoiyh so OPENSSL_cleanup() gets called
# at application exit.
#
# For example it is futile openssl-3.3 (and earlier) calls OPENSSL_cleanup()
# when we run ./test.implicit with no arguments. In this case openssl library
# does not initialize atexit hanler at all.
#
# Running test using command './test.implicit -a base64AtExit' makes test to
# initialize openssl library enough (byond basic initializtion) so atexit
# handler is set to invoke OPENSSL_cleanup() when application exits. The patched
# library will prouce 'OPENSSL_cleanup` to standard ouptput at application exit.
#
# Using '-a base64AtExit' option does not matter on master branch (openssl-3.4)
# where call to OPENSSL_cleanup() is moved to library destructor.
# 

include Makefile.in
