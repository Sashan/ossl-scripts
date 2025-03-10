#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>


static void *allocate_by_realloc = NULL;
static void *allocate_by_malloc = NULL;
static void *free_by_realloc = NULL;


int
main(int argc, const char *argv[])
{
	void *tmp;

	allocate_by_malloc = CRYPTO_malloc(256, __FILE__, __LINE__);
	allocate_by_realloc = CRYPTO_realloc(allocate_by_realloc, 256,
	    __FILE__, __LINE__);
	free_by_realloc = CRYPTO_malloc(512, __FILE__, __LINE__);

	/* change sizes by realloc */
	tmp = CRYPTO_realloc(allocate_by_realloc, 256 * 1024,
	    __FILE__, __LINE__);
	if (tmp != NULL)
		allocate_by_realloc = tmp;

	tmp  = CRYPTO_realloc(allocate_by_malloc, 128,
	    __FILE__, __LINE__);
	if (tmp != NULL)
		allocate_by_malloc = tmp;

	CRYPTO_free(allocate_by_realloc, __FILE__, __LINE__);
	free_by_realloc = CRYPTO_realloc(free_by_realloc, 0,
	    __FILE__, __LINE__);
	CRYPTO_free(allocate_by_malloc, __FILE__, __LINE__);

	allocate_by_malloc = CRYPTO_malloc(256, __FILE__, __LINE__);
	CRYPTO_free(allocate_by_malloc, __FILE__, __LINE__);

        return (0);
}
