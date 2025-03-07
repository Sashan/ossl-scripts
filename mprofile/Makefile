#
# use options below when compiling with gcc
#
CPPFLAGS=-I/usr/lib/gcc/x86_64-linux-gnu/13/include
CPPFLAGS+=-D_GNU_SOURCE
LDFLAGS+=-ldl
LDFLAGS+=-L/usr/lib/gcc/x86_64-linux-gnu/13/
LDFLAGS+=-lgcc_s

##
## use options below when building with clang on OpenBSD
##
## pulls libunwind.h header supplied by clang
## perhaps specific to OpenBSD.
##
#CPPFLAGS=-I/usr/include/c++/v1/
#CPPFLAGS+=-DUSE_LIBUNWIND
##
## This is perhaps specific to OpenBSD. Other
## systems might differ. -lLLVM pulls all
## libraries which come form clang including libunwind
## For other systems you need to bit of googling.
## last resort is to build libunind on your own:
##	https://github.com/libunwind/libunwind
## you may use clang options:
##	 --rtlib=compiler-rt --unwindlib=libunwind
## as described here:
##	https://maskray.me/blog/2020-11-08-stack-unwinding
##
#LDFLAGS=-lLLVM

#
# common options to add libcrypto
#
CPPFLAGS+=-D_WITH_STACKTRACE
CPPFLAGS+=-fPIC
CPPFLAGS+=-I$(OPENSSL_HEADERS)
OSSLLIB=$(OPENSSL_LIB_PATH)

all: libmprofile.so

init.o: init.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o init.o init.c

ksyms.o: ksyms.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o ksyms.o ksyms.c

record.o: record.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o record.o record.c

stack.o: stack.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o stack.o stack.c

libmprofile.so: init.o ksyms.o record.o stack.o
	$(CC) -shared -fPIC -o libmprofile.so init.o record.o \
	    stack.o ksyms.o -lelf $(LDFLAGS) -L$(OSSLLIB) -lcrypto

clean:
	rm -f *.o
	rm -f libmprofileso
