#include <stdlib.h>
#include <openssl/crypto.h>

#include "mprofile.h"

static void *mp_CRYPTO_malloc(size_t, const char *, int);
static void mp_CRYPTO_free(void *, const char *, int);
static void *mp_CRYPTO_realloc(void *, size_t, const char *, int);

mprofile_t	*mp = NULL;	/* profile should be per thread */

static void __attribute__ ((constructor)) init(void);

static void
save_profile(void)
{
	mprofile_load_syms(mp);
	mprofile_save(mp);
	mprofile_destroy(mp);
	mprofile_done();
}

/*
 * We need to fire at exit, so all shared libraries are still loaded,
 * so we will be able to resolve symbols.
 */
static void
init(void)
{
	mp = mprofile_create();
	CRYPTO_set_mem_functions(mp_CRYPTO_malloc, mp_CRYPTO_realloc,
	    mp_CRYPTO_free);
	atexit(save_profile);
}

#ifdef _WITH_STACKTRACE

#ifdef USE_LIBUNWIND

#include <libunwind.h>

static void
collect_backtrace(mprofile_stack_t *mps)
{
	unw_cursor_t uw_cursor;
	unw_context_t uw_context;;
	unw_word_t fp;

	unw_getcontext(&uw_context);
	unw_init_local(&uw_cursor, &uw_context);

	do {
		unw_get_reg(&uw_cursor, UNW_REG_IP, &fp);
		mprofile_push_frame(mps, (unsigned long long)fp);
	} while (unw_step(&uw_cursor) > 0);

}
#else	/* !USE_LIBUNWIND */
#include <unwind.h>

static _Unwind_Reason_Code
collect_backtrace(struct _Unwind_Context *uw_context, void *cb_arg)
{
	unsigned long long fp = _Unwind_GetIP(uw_context);
	mprofile_stack_t *mps = (mprofile_stack_t *)cb_arg;

	mprofile_push_frame(mps, fp);

	return (_URC_NO_REASON);
}
#endif	/* USE_LIBUNWIND */
#endif

void *
mp_CRYPTO_malloc(size_t sz, const char *file, int line)
{
	void *rv;
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));
#else
	mprofile_stack_t *mps = NULL;
#endif

	rv = malloc(sz);
#ifdef _WITH_STACKTRACE
#ifdef USE_LIBUNWIND
	collect_backtrace(mps);
#else
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
#endif	/* _WITH_STACKTRACE */
	mprofile_record_alloc(mp, rv, sz, mps);

	return (rv);
}

void
mp_CRYPTO_free(void *b, const char *file, int line)
{
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));
#else
	mprofile_stack_t *mps = NULL;
#endif

#ifdef _WITH_STACKTRACE
#ifdef USE_LIBUNWIND
	collect_backtrace(mps);
#else
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
#endif	/* _WITH_STACKTRACE */
	mprofile_record_free(mp, b, mps);
	free(b);
}

void *
mp_CRYPTO_realloc(void *b, size_t sz, const char *file, int line)
{
	void *rv;
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));;
#else
	mprofile_stack_t *mps = NULL;
#endif

	rv = realloc(b, sz);
	if (rv == NULL)
		return (NULL);	/* consider recording failure */

#ifdef _WITH_STACKTRACE
#ifdef USE_LIBUNWIND
	collect_backtrace(mps);
#else
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
#endif	/* _WITH_STACKTRACE */
	if (sz == 0)
		mprofile_record_free(mp, rv, mps);
	else
		mprofile_record_realloc(mp, rv, sz, b, mps);

	return (rv);
}
