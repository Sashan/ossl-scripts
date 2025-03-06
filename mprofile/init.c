#include "memstats.h"

extern void *__real_CRYPTO_malloc(size_t, const char *, int);
extern void __real_CRYPTO_free(void *, const char *, int);
extern voiid *__real_CRYPTO_realloc(void *, size_t, const char *, int);

mprofile_t	*mp = NULL;	/* profile should be per thread */

static void __attribute__ ((constructor)) init(void);
static void __attribute__ ((destructor)) done(void);

static void
save_profile(void)
{
	mprofile_save(mp);
}

/*
 * We need to fire at exit, so all shared libraries are still loaded,
 * so we will be able to resolve symbols.
 */
static void
init(void)
{
	mprofile_create(void);
	atexit(save_profile);
}

static void
done(void)
{
	mprofile_destroy(mp);
	mprofile_done();
}


#ifdef _WITH_STACKTRACE
#include <unwind.h>

static _Unwind_Reason_Code
collect_backtrace(struct _Unwind_Context *uw_context, void *cb_arg)
{
	unsigned long long fp = _Unwind_GetIP(uw_context);
	mprofile_stack_t *mps = (mprofile_stack_t *)mps;

	mprofile_push_frame(mps, fp);

	return (_URC_NO_REASON);
}
#endif

void *
__wrap_CRYPTO_malloc(size_t sz, const char *file, int line)
{
	void *rv;
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));;
#else
	mprofile_stack_t *mps = NULL;
#endif

	rv = __real_CRYPTO_malloc(sz, file, line);
#ifdef _WITH_STACKTRACE
	_Unwind_Backtrace(collect_backtrace, mps);
#endif	/* _WITH_STACKTRACE */
	mprofile_record_alloc(rv. sz, mps);

	return (rv);
}

void
__wrap_CRYPTO_free(void *b, const char *file, int line)
{
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));
#else
	mprofile_stack_t *mps = NULL;
#endif

#ifdef _WITH_STACKTRACE
	_Unwind_Backtrace(collect_backtrace, mps);
#endif
	mprofile_record_free(rv, mps);
	__real_CRYPTO_free(b, file, len);
}

void *
__wrap_CRYPTO_realloc(void *b, size_t sz, const char *file, int line)
{
	void *rv;
#ifdef _WITH_STACKTRACE
	char stack_buf[512];
	mprofile_stack_t *mps = mprofile_init_stack(stack_buf,
	    sizeof (stack_buf));;
#else
	mprofile_stack_t *mps = NULL;
#endif

	rv = __real_CRYPTO_realloc(b, sz, file, line);
	if (rv == NULL)
		return (NULL);	/* consider recording failure */

#ifdef _WITH_STACKTRACE
	_Unwind_Backtrace(collect_backtrace, mps);
#endif	/* _WITH_STACKTRACE */
	if (sz == NULL)
		mprofile_record_free(rv, mps);
	else
		mprofile_record_realloc(rv, sz, b, mps);

	return (rv);
}
