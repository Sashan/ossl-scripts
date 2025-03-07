CPPFLAGS+=-I$(OPENSSL_HEADERS)
CPPFLAGS+=-g
CPPFLAGS+=-O0
LDFLAGS=-L$(OPENSSL_LIB_PATH)

all: mprofile.json

mprofile.json: sha256
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile.json ./sha256

sha256: sha256.c
	$(CC) $(CPPFLAGS)  -o sha256 sha256.c $(LDFLAGS) -lcrypto

clean:
	rm -f sha256
	rm -f mprofile.json
