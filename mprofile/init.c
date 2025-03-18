#include <stdlib.h>
#include <pthread.h>
#include <openssl/crypto.h>

#include "mprofile.h"

static void *mp_CRYPTO_malloc(size_t, const char *, int);
static void mp_CRYPTO_free(void *, const char *, int);
static void *mp_CRYPTO_realloc(void *, size_t, const char *, int);

struct memhdr {
	size_t		mh_size;
	uint64_t	mh_chk;
};

static pthread_key_t mp_pthrd_key;

static void __attribute__ ((constructor)) init(void);

static void
merge_profile(void *void_mprof)
{
	mprofile_t	*mp = (mprofile_t *)void_mprof;
	/* profiles are freed in save_profile() on behalf of atexit() */
	pthread_setspecific(mp_pthrd_key, NULL);
}

static void
save_profile(void)
{
	pthread_key_delete(mp_pthrd_key);
	mprofile_merge();
	mprofile_done();
}

static mprofile_t *
get_mprofile(void)
{
	mprofile_t *mp = (mprofile_t *)pthread_getspecific(mp_pthrd_key);

	if (mp == NULL) {
		mp = mprofile_create();
		mprofile_add(mp);
		pthread_setspecific(mp_pthrd_key, mp);
	}

	return (mp);
}

/*
 * We need to fire at exit, so all shared libraries are still loaded,
 * so we will be able to resolve symbols.
 */
static void
init(void)
{
	mprofile_init();
	pthread_key_create(&mp_pthrd_key, merge_profile);
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
	struct memhdr *mh;
	void *rv;
	mprofile_t *mp = get_mprofile();
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));
#else
	mprofile_stack_t *mps = NULL;
#endif

	mh = (struct memhdr *) malloc(sz + sizeof (struct memhdr));
	if (mh != NULL) {
		mh->mh_size = sz;
		mh->mh_chk = (uint64_t)mh ^ sz;
		rv = (void *)((char *)mh + sizeof (struct memhdr));
	} else {
		rv = NULL;
	}
#ifdef _WITH_STACKTRACE
#ifdef USE_LIBUNWIND
	collect_backtrace(mps);
#else
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
#endif	/* _WITH_STACKTRACE */
	if (mp != NULL)
		mprofile_record_alloc(mp, rv, sz, mps);

	return (rv);
}

void
mp_CRYPTO_free(void *b, const char *file, int line)
{
	struct memhdr *mh = NULL;
	mprofile_t *mp = get_mprofile();
	uint64_t chk;
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

	if (b != NULL) {
		mh = (struct memhdr *)((char *)b - sizeof (struct memhdr));
		chk = (uint64_t)mh ^ mh->mh_size;
		if (chk != mh->mh_chk) {
			fprintf(stderr, "%p memory corruption detected in %s!",
			    b, __func__);
			abort();
		}
	}

	if (mp != NULL)
		mprofile_record_free(mp, b, (b == NULL) ? 0 : mh->mh_size, mps);
	free(mh);
}

void *
mp_CRYPTO_realloc(void *b, size_t sz, const char *file, int line)
{
	struct memhdr *mh;
	struct memhdr save_mh;
	uint64_t chk;
	mprofile_t *mp = get_mprofile();
	void *rv = NULL;
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));;
#else
	mprofile_stack_t *mps = NULL;
#endif

	if (b != NULL) {
		mh = (struct memhdr *)((char *)b - sizeof (struct memhdr));
		chk = (uint64_t)mh ^ mh->mh_size;
		if (chk != mh->mh_chk) {
			fprintf(stderr, "%p memory corruption detected in %s!",
			    b, __func__);
			abort();
		}
		save_mh = *mh;
	} else {
		mh = NULL;
	}

	if (sz == 0)
		mprofile_record_free(mp, b, (b == NULL) ? 0 : mh->mh_size, mps);

	mh = (struct memhdr *)realloc(mh, (sz != 0) ?
	    sz + sizeof (struct memhdr) : 0);
	if (mh == NULL)
		return (NULL);	/* consider recording failure */

	if (sz == 0)
		return (b);

#ifdef _WITH_STACKTRACE
#ifdef USE_LIBUNWIND
	collect_backtrace(mps);
#else
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
#endif	/* _WITH_STACKTRACE */
	rv = (void *)((char *)mh + sizeof (struct memhdr));
	if (mp != NULL) {
		mh->mh_size = sz;
		mh->mh_chk = (uint64_t)mh ^ sz;
		if (b == NULL) {
			mprofile_record_alloc(mp, rv, sz, mps);
		} else {
			mprofile_record_realloc(mp, rv, sz,
			    save_mh.mh_size, b, mps);
		}
	}

	return (rv);
}
