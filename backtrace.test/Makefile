CPPFLAGS=-I/usr/lib/gcc/x86_64-linux-gnu/13/include
CPPFLAGS+=-I$(OPENSSL_HEADERS)
CPPFLAGS+=-D_GNU_SOURCE
CPPFLAGS+=-fPIC
LDFLAGS=-L/usr/lib/gcc/x86_64-linux-gnu/13/

all: backtrace

main.o: main.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o main.o main.c

ksyms.o: ksyms.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o ksyms.o ksyms.c

backtrace.o: backtrace.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o backtrace.o backtrace.c

libmain.o: libmain.c
	$(CC) $(CPPFLAGS) -c -O0 -g -o backtrace.o backtrace.c

backtrace: main.o ksyms.o backtrace.o
	$(CC) -o backtrace main.o ksyms.o backtrace.o -ldl -lelf \
	    $(LDFLAGS) -lgcc_s -L$(OPENSSL_LIB_PATH) -lcrypto

libbacktrace.so: libmain.o backtrace.o ksyms.o
	$(CC) -shared -fPIC -o libbacktrace.so libbacktrace.c

clean:
	rm -f *.o
	rm -f backtrace libbacktrace.so
