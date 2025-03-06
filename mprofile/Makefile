CPPFLAGS=-I/usr/lib/gcc/x86_64-linux-gnu/13/include
CPPFLAGS+=-D_GNU_SOURCE
CPPFLAGS+=-D_WITH_STACKTRACE
CPPFLAGS+=-fPIC
CPPFLAGS+=-I$(OPENSSL_HEADERS)
LDFLAGS=-L/usr/lib/gcc/x86_64-linux-gnu/13/
OSSLLIB=-L$(OPENSSL_LIB_PATH)

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
	    stack.o ksyms.o -ldl -lelf $(LDFLAGS) -lgcc_s \
	    -L$(OPENSSL_LIB_DIR) -lcrypto

clean:
	rm -f *.o
	rm -f libmprofileso
