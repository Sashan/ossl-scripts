CPPFLAGS+=-I$(OPENSSL_HEADERS)
CPPFLAGS+=-g
CPPFLAGS+=-O0
LDFLAGS=-L$(OPENSSL_LIB_PATH)

all:	mprofile-sha256-stats.json \
	mprofile-sha256-chains.json \
	mprofile-sha256-stacks.json \
 	mprofile-realloc-stats.json \
 	mprofile-realloc-chains.json \
 	mprofile-realloc-stacks.json

mprofile-sha256-stats.json: sha256
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-sha256-stats.json \
	    MPROFILE_MODE=1 ./sha256

mprofile-sha256-chains.json: sha256
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-sha256-chains.json \
	    MPROFILE_MODE=2 ./sha256

mprofile-sha256-stacks.json: sha256
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-sha256-stacks.json \
	    MPROFILE_MODE=3 ./sha256

mprofile-realloc-stats.json: realloc
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-realloc-stats.json \
	    MPROFILE_MODE=1 ./realloc

mprofile-realloc-chains.json: realloc
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-realloc-chains.json \
	    MPROFILE_MODE=2 ./realloc

mprofile-realloc-stacks.json: realloc
	LD_PRELOAD=../libmprofile.so MPROFILE_OUTF=./mprofile-realloc-stacks.json \
	    MPROFILE_MODE=3 ./realloc

sha256: sha256.c
	$(CC) $(CPPFLAGS)  -o sha256 sha256.c $(LDFLAGS) -lcrypto

realloc: realloc.c
	$(CC) $(CPPFLAGS)  -o realloc realloc.c $(LDFLAGS) -lcrypto

clean:
	rm -f sha256 realloc
	rm -f mprofile-sha256-*.json mprofile-realloc-*.json
