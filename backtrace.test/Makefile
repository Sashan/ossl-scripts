CPPFLAGS=-I/usr/lib/gcc/x86_64-linux-gnu/13/include
CPPFLAGS+=-I$(OPENSSL_HEADERS)
CPPFLAGS+=-D_GNU_SOURCE
LDFLAGS=-L/usr/lib/gcc/x86_64-linux-gnu/13/

all: backtrace

main.o: main.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o main.o main.c

ksyms.o: ksyms.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o ksyms.o ksyms.c

backtrace: main.o ksyms.o
	$(CC) -o backtrace main.o ksyms.o -ldl -lelf \
	    $(LDFLAGS) -lgcc_s -L$(OPENSSL_LIB_PATH) -lcrypto

clean:
	rm -f *.o
	rm backtrace
