CPPFLAGS+=-I$(OPENSSL_HEADERS)
CPPFLAGS+=-g
CPPFLAGS+=-O0
LDFLAGS=-L$(OPENSSL_LIB_PATH)

all: mprofile-sha256.json mprofile-realloc.json

mprofile-sha256.json: sha256
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-sha256.json \
	    ./sha256

mprofile-realloc.json: realloc
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-realloc.json \
	    ./realloc

sha256: sha256.c
	$(CC) $(CPPFLAGS)  -o sha256 sha256.c $(LDFLAGS) -lcrypto

realloc: realloc.c
	$(CC) $(CPPFLAGS)  -o realloc realloc.c $(LDFLAGS) -lcrypto

clean:
	rm -f sha256 realloc
	rm -f mprofile-sha256.json mprofile-realloc.json
