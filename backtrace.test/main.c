#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <openssl/evp.h>

#include "kelf.h"

#define	MAX_STACK_DEPTH	64
static unsigned long long  stack[MAX_STACK_DEPTH];
static unsigned long long *top = stack;
struct syms *syms = NULL;

struct shlib {
	char *shl_name;
	unsigned long shl_base;
};

static struct shlib shlibs[MAX_STACK_DEPTH];
/*
 * Write is signal safe.
 */
static void
write_hex(unsigned long long x)
{
	unsigned long long mask;
	static char	*hex_tab[16] = {
	    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	    "a", "b", "c", "e", "d", "f"
	};
	int	i;

	mask = (unsigned long long int)0xf << ((sizeof (x) * 8) - 4);

	write(1, "0x", 2);
	/*
	 * mask and shift from the first MSB bits
	 */
	for (i = ((sizeof (x) * 8) / 4) ; i > 0; i--) {
		write(1, hex_tab[(x & mask) >> ((i - 1) * 4)], 1);
		mask = mask >> 4;
	}
}

static void
add_shlib(Dl_info *dli)
{
	unsigned int i = 0;

	while (shlibs[i].shl_name != NULL && i < MAX_STACK_DEPTH) {
		if (strcmp(shlibs[i].shl_name, dli->dli_fname) == 0)
			return;
		i++;
	}

	if (i == MAX_STACK_DEPTH)
		return;

	shlibs[i].shl_name = strdup(dli->dli_fname);
	shlibs[i].shl_base = (uintptr_t)dli->dli_fbase;
}

#ifdef	_LIBUNWIND

#include <libunwind.h>

void
sigtrap_hndl(int signum)
{
	unw_cursor_t uw_cursor;
	unw_context_t uw_context;;
	nw_word_t ip, sp;

	unw_getcontext(&uw_context);
	unw_init_local(&uw_cursor, &uw_context);

	do {
		unw_get_reg(&uw_cursor, UNW_REG_IP, &ip);
		unw_get_reg(&uw_cursor, UNW_REG_SP, &sp);
		write(1, "ip = 0x", 6);
		write_hex((unsigned long long)ip);
		write(1, ", sp = 0x", 6); 
		write_hex((unsigned long long)sp);
		write(1, "\n", 1); 
	} while (unw_step(&uw_cursor) > 0);
}
#else
#include <unwind.h>

static _Unwind_Reason_Code
print_frame(struct _Unwind_Context *uw_context, void *cb_arg)
{
	unsigned long long fp = _Unwind_GetIP(uw_context);

	*top = fp;
	top++;
	write_hex(fp);
	write(1, "\n", 1);

	return (_URC_NO_REASON);
}

void
sigtrap_hndl(int signum)
{
	top = stack;
	_Unwind_Backtrace(print_frame, NULL);
}
#endif

void
load_syms(void)
{
	Dl_info	dli;
	unsigned long long *walk = stack;
	unsigned int i = 0;

	while (walk < top) {
		if (dladdr((void *)*walk, &dli) != 0)
			add_shlib(&dli);
		walk++;
	}

	while ((i < MAX_STACK_DEPTH) && (shlibs[i].shl_name != NULL)) {
		syms = kelf_open(shlibs[i].shl_name, syms, shlibs[i].shl_base);
		i++;
	}
}

void
print_stack(void)
{
	Dl_info	dli;
	unsigned long long *walk = stack;
	char buf[80];

	load_syms();

	while (walk < top) {
		kelf_snprintsym(syms, buf, sizeof (buf), *walk, 0);
		printf("%s", buf);
		walk++;
	}
}

void
do_sha256(void)
{
	unsigned char	 md_value[EVP_MAX_MD_SIZE];
	EVP_MD		*md_sha256 = EVP_MD_fetch(NULL, "SHA256", NULL);
	EVP_MD_CTX	*md_ctx = EVP_MD_CTX_new();
	unsigned int	 md_len;

	if (md_ctx == NULL || md_sha256 == NULL)
		goto done;

	memset(md_value, 0, sizeof (md_value));
	EVP_DigestInit_ex2(md_ctx, md_sha256, NULL);
	EVP_DigestUpdate(md_ctx, "Part 1",sizeof ("Part 1") - 1);
	EVP_DigestUpdate(md_ctx, "Part 2",sizeof ("Part 2") - 1);
	EVP_DigestFinal_ex(md_ctx, md_value, &md_len);
done:
	EVP_MD_CTX_free(md_ctx);
	EVP_MD_free(md_sha256);
}

int
main(int argc, const char *argv[])
{
        struct sigaction sa;

	/*
	 * Initialize SIGTRAP handler. We expect to see upon execution of int3
	 * instruction.  The handler then should do the stack unwind.
	 */
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = sigtrap_hndl;
        sigaction(SIGTRAP, &sa, NULL);

	do_sha256();

	print_stack();

	kelf_close(syms);

        return (0);
}
