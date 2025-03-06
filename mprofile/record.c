#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>

#include "utils/queue.h"
#include "kelf.h"

#include "mprofile.h"

enum {
	ALLOC = 1,
	FREE = 2,
	REALLOC = 3
};

#define	MPROFILE_REC_MEM	"\"addr\""
#define	MPROFILE_REC_REALLOC	"\"realloc\""
#define	MPROFILE_REC_SZ		"\"rsize\""
#define	MPROFILE_REC_STATE	"\"state\""
#define	MPROFILE_REC_STACK_ID	"\"stack_id\""
#define	MPROFILE_TIMESTAMP	"\"time\""
#define	MPROFILE_TIME_S		"\"s\""
#define	MPROFILE_TIME_NS	"\"ns\""
struct mprofile_record {
	void				*mpr_mem;
	void				*mpr_realloc;
	size_t	 			 mpr_sz;
	char				 mpr_state;
	unsigned int			 mpr_stack_id;
	struct timespec			 mpr_ts;
	LIST_ENTRY(mprofile_record)	 mpr_le;
};

struct mprofile {
	LIST_HEAD(mp_list, mprofile_record)	 mp_lhead;
	mprofile_stset_t			*mp_stset;
};

struct shlib {
	char		*shl_name;
	unsigned long	 shl_base;
};

#define	MAX_SHLIBS	32
static struct shlib shlibs[MAX_SHLIBS];

/* we keep symbols global */
static struct syms *syms = NULL;

static struct mprofile_record *
create_mprofile_record(void)
{
	struct mprofile_record *mpr;

	mpr = (struct mprofile_record *)malloc(
	    sizeof (struct mprofile_record));
	if (mpr == NULL)
		return (NULL);
	memset(mpr, 0, sizeof (struct mprofile_record));

	clock_gettime(CLOCK_REALTIME, &mpr->mpr_ts);

	return (mpr);
}

static void
print_mprofile_record(FILE *f, struct mprofile_record *mpr)
{
	const char *state;

	switch (mpr->mpr_state) {
	case ALLOC:
		state = "\"allocated\"";
		break;
	case FREE:
		state = "\"free\"";
		break;
	case REALLOC:
		state = "\"realloc\"";
		break;
	default:
		state = "\"???\"";
	}
	fprintf(f, "\t{\n");
	fprintf(f, "\t\t%s : %llu,\n", MPROFILE_REC_MEM,
	    (unsigned long long)mpr->mpr_mem);
	fprintf(f, "\t\t%s : %llu,\n", MPROFILE_REC_REALLOC,
	    (unsigned long long)mpr->mpr_realloc);
	fprintf(f, "\t\t%s : %zu,\n", MPROFILE_REC_SZ, mpr->mpr_sz);
	fprintf(f, "\t\t%s : %s,\n", MPROFILE_REC_STATE, state);
	fprintf(f, "\t\t%s : %u,\n", MPROFILE_REC_STACK_ID,
	    mpr->mpr_stack_id);
	fprintf(f, "\t\t%s : {\n", MPROFILE_TIMESTAMP);
	fprintf(f, "\t\t\t%s : %lu,\n", MPROFILE_TIME_S, mpr->mpr_ts.tv_sec);
	fprintf(f, "\t\t\t%s : %lu\n", MPROFILE_TIME_NS,
	    mpr->mpr_ts.tv_nsec);
	fprintf(f, "\t\t}\n");
}

mprofile_t *
mprofile_create(void)
{
	mprofile_t *mp;

	mp = (mprofile_t *) malloc(sizeof (mprofile_t));
	if (mp == NULL)
		return (NULL);

	LIST_INIT(&mp->mp_lhead);

#ifdef _WITH_STACKTRACE
	mp->mp_stset = mprofile_create_stset();
#endif

	return (mp);
}

#ifdef _WITH_STACKTRACE
static void
print_trace(unsigned long long frame, void *f_arg)
{
	FILE *f = (FILE *)f_arg;
	char buf[90];

	kelf_snprintsym(syms, buf, sizeof (buf), frame, 0);
	fprintf(f, "\"%s\", ", buf);
}

static void
print_stack(FILE *f, mprofile_stack_t *stack)
{
	fprintf(f, "\t{\n");
	fprintf(f, "\t\t\"stack_id\" : %u,\n", mprofile_get_stack_id(stack));
	fprintf(f, "\t\t\"stack_count\" : %u,\n",
	    mprofile_get_stack_count(stack));
	fprintf(f, "\t\t\"stack_trace\" : [ ");
	mprofile_walk_stack(stack, print_trace, f);
	fprintf(f, "\"\" ]\n");
	fprintf(f, "\t}");
}
#endif

void
mprofile_save(mprofile_t *mp)
{
	unsigned int i;
	char *fname = getenv("MPROFILE_OUTF");
	FILE *f;
	mprofile_stack_t *stack;
	struct mprofile_record *mpr;
	int first = 1;

	if (fname == NULL)
		return;

	f = fopen(fname, "w");
	if (f == NULL)
		return;

	fprintf(f, "[\n");
	LIST_FOREACH(mpr, &mp->mp_lhead, mpr_le) {
		if (first == 0) {
			fprintf(f, "\t},\n");
			first = 0;
		}
		print_mprofile_record(f, mpr);
	}
	if (first == 0)
		fprintf(f, "\t}\n");
	fprintf(f, "]");

#ifdef _WITH_STACKTRACE
	fprintf(f, ",\n[\n");
	stack = mprofile_get_next_stack(mp->mp_stset, NULL);
	while (stack != NULL) {
		print_stack(f, stack);
		stack = mprofile_get_next_stack(mp->mp_stset, stack);
		if (stack != NULL)
			fprintf(f, ",\n");
	}
	fprintf(f, "\n]");
#endif

	fclose(f);
}

void
mprofile_destroy(mprofile_t *mp)
{
	unsigned int i;
	struct mprofile_record *mpr, *walk;

	LIST_FOREACH_SAFE(mpr, &mp->mp_lhead, mpr_le, walk) {
		LIST_REMOVE(mpr, mpr_le);
		free(mpr);
	}
	mprofile_destroy_stset(mp->mp_stset);
	free(mp);
}

void
mprofile_record_alloc(mprofile_t *mp, void *buf, size_t buf_sz,
    mprofile_stack_t *mps)
{
	struct mprofile_record *mpr;

	mpr = create_mprofile_record();
	if (mpr == NULL)
		return;

	mpr->mpr_mem = buf;
	mpr->mpr_sz = buf_sz;
	mpr->mpr_state = ALLOC;
	LIST_INSERT_HEAD(&mp->mp_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mps = mprofile_add_stack(mp->mp_stset, mps);
	mpr->mpr_stack_id = mprofile_get_stack_id(mps);
#endif
}

void
mprofile_record_free(mprofile_t *mp, void *buf, mprofile_stack_t *mps)
{
	struct mprofile_record *mpr;

	mpr = create_mprofile_record();
	if (mpr == NULL)
		return;

	mpr->mpr_mem = buf;
	mpr->mpr_sz = 0;
	mpr->mpr_state = FREE;
	LIST_INSERT_HEAD(&mp->mp_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mps = mprofile_add_stack(mp->mp_stset, mps);
	mpr->mpr_stack_id = mprofile_get_stack_id(mps);
#endif
}

void
mprofile_record_realloc(mprofile_t *mp, void *buf, size_t buf_sz,
    void *old_buf, mprofile_stack_t *mps)
{
	struct mprofile_record *mpr;

	mpr = create_mprofile_record();
	if (mpr == NULL)
		return;

	mpr->mpr_mem = buf;
	mpr->mpr_sz = buf_sz;
	mpr->mpr_realloc = old_buf;
	mpr->mpr_state = REALLOC;
	LIST_INSERT_HEAD(&mp->mp_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mps = mprofile_add_stack(mp->mp_stset, mps);
	mpr->mpr_stack_id = mprofile_get_stack_id(mps);
#endif
}

static void
add_shlib(Dl_info *dli)
{
	unsigned int i = 0;

	while (shlibs[i].shl_name != NULL && i < MAX_SHLIBS) {
		if (strcmp(shlibs[i].shl_name, dli->dli_fname) == 0)
			return;
		i++; }

	if (i == MAX_SHLIBS)
		return;

	shlibs[i].shl_name = strdup(dli->dli_fname);
	shlibs[i].shl_base = (uintptr_t)dli->dli_fbase;
}

static void
walk_stack(unsigned long long frame, void *arg)
{
	Dl_info dli;

	if (dladdr((uintptr_t *)frame, &dli) != 0)
		add_shlib(&dli);
}

/*
 * This is expensive, we walk trhough all stacks
 * to find list of shared libraries. There must
 * better way to obtain the list of shared libraries.
 */
void
mprofile_load_syms(mprofile_t *mp)
{
#ifdef _WITH_STACKTRACE
	unsigned int	i;
	mprofile_stack_t *stack;

	stack = mprofile_get_next_stack(mp->mp_stset, NULL);
	while (stack != NULL) {
		mprofile_walk_stack(stack, walk_stack, NULL);
		stack = mprofile_get_next_stack(mp->mp_stset, stack);
	}

	for (i = 0; i < MAX_SHLIBS; i++) {
		if (shlibs[i].shl_name != NULL)
			syms = kelf_open(shlibs[i].shl_name, syms, shlibs[i].shl_base);
	}
#endif
}

void
mprofile_done(void)
{
	unsigned int	i;

	for (i = 0; i < MAX_SHLIBS; i++)
		free(shlibs[i].shl_name);

	kelf_close(syms);
}
