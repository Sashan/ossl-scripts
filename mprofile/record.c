#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "utils/queue.h"
#include "utils/tree.h"
#include "utils/kelf.h"

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
#define	MPROFILE_TIMESTAMP	"\"time\""
#define	MPROFILE_TIME_S		"\"s\""
#define	MPROFILE_TIME_NS	"\"ns\""
struct mprofile_record {
	void				*mpr_mem;
	void				*mpr_realloc;
	size_t	 			 mpr_sz;
	char				 mpr_state;
	struct timespec			 mpr_ts;
	LIST_ENTRY(mprofile_record)	 mpr_le;
};

#define	MPROFILE_RS_SZ		"\"buf_sz\""
#define	MPROFILE_RS_BUFLIST	"\"buf_list\""
#define	MPROFILE_RS_STACKS	"\"stack_list\""
struct mprofile_record_set {
	size_t				 mrs_sz;
	LIST_HEAD(mprofile_record)	 mrs_lhead;
	mprofile_stack_set_t		*mrs_stset;
	RB_ENTRY(mprofile_record_set)	 mrs_rbe;
};

struct mprofile_entry {
	size_t			mpe_max_sz;
	RB_HEAD(mprofile_entry)	mpe_rbh;
}

#define	MPROFILE_COUNT	32
struct mprofile {
	unsigned int		mp_cnt;
	struct mprofile_entry	mp_entries[MPROFILE_COUNT];
};

struct shlib {
	char *shl_name,
	unsigned long shl_base;
};

#define	MAX_SHLIBS	32
static struct shlib slibs[MAX_SHLIBS];

/* we keep symbols global */
static struct ksyms *syms = NULL;

static int compare_record_set(struct mprofile_record_set *,
    struct mprofile_record_set *);

RB_GENERATE_STATIC(mprofile_entry, mprofile_record_set, mrs_rbe,
    compare_record_set);

static int
compare_record_set(struct mprofile_record_set *a_mrs,
    struct mprofile_record_set *b_mrs)
{
	if (a_mrs->mrs_sz < b_mrs->mrs_sz)
		return (-1);
	else if (a_mrs->mrs_sz > b_mrs->mrs_sz)
		return (1);
	else
		return (0);
}

static void
destroy_record_set(struct mprofile_record_set *mrs)
{
	struct mprofile_record *mpr, *walk;

#ifdef _WITH_STACKTRACE
	mprofile_destroy_stset(mrs->mrs_stset);
#endif
	LIST_FOREACH_SAFE(mpr, &mrs->mrs_lhead, mpr_le, walk) {
		LIST_REMOVE(mpr, mpr_le);
		free(mpr);
	}

	free(mrs);
}

static void
destroy_entry(struct mprofile_entry *mpe)
{
	struct mprofile_record_set *mrs, *walk;

	RB_FOREACH_SAFE(mrs, mprofile_entry, &mpe->mpe_rbh, walk) {
		RB_REMOVE(mprofile_entry, mrs);
		destroy_record_set(mrs);
	}
}

static struct mprofile_entry *
find_entry_for_size(mprofile_t *mp, size_t sz)
{
	unsigned int i;

	for (i = 0; i < mp->mp_cnt; i++)
		if (sz > mp->mp_entries[i].mpe_max_sz)
			break;

	if (i == mp->mp_cnt)
		i--;

	return (mp->mp_entries[i])
}

static struct mprofile_record_set *
find_record_set_for_size(mprofile_entry *mpe, size_t sz)
{
	struct mprofile_record_set	key_mpe;

	key_mrs.mrs_sz = sz;
	return (RB_FIND(mprofile_record_set, &mpe->mpe_rbh, &key_mrs));
}

static struct mprofile_record_set *
create_record_set_for_size(mprofile_entry *mpe, size_t sz)
{
	struct mprofile_record_set *mrs;

	mrs = (struct mprofile_record_set *) malloc(
	    sizeof (struct mprofile_record_set));
	if (mrs == NULL)
		return (NULL);

	mrs->mrs_sz = sz;
	LIST_INIT(&mrs->mrs_lhead);
	RB_INSERT(mprofile_record_set, &mpe->mpe_rbh, mrs);

#ifdef _WITH_STACKTRACE
	mrs->mrs_stset = mprofile_create_stset();
#endif
	return (mrs);
}

static mprofile_record *
create_record(void)
{
	struct mprofile_record *mpr;

	mpr = (struct mprofile_record *)malloc(
	    sizeof (struct mprofile_record));
	if (mpr == NULL)
		return (NULL);
	memset(mpr, 0, sizeof (struct mprofile_record));

	clock_gettime(CLOCK_REALTIME, &mpr->mpr_ts);
	return mpr;
}

void
print_trace(unsigned long long frame, void *f_arg)
{
	FILE *f = (FILE *)f_arg;
	char buf[90];

	kelf_snprintsym(syms, bud, sizeof (buf), frame, 0)
	fprintf(f, "\"%s\", ", buf);
}

#ifdef _WITH_STACKTRACE
static void
print_stack(FILE *f, mprofile_stack_t *stack)
{
	fprintf(f, "\t\t{\n");
	fprintf(f, "\t\t\t\"stack_id\" : %u,\n", mprofile_get_stack_id(stack);
	fprintf(f, "\t\t\t\"stack_count\" : %u,\n",
	    mprofile_get_stack_count(stack);
	fprintf(f, "\t\t\t\"stack_trace\" : [ ",
	mprofile_walk_stack(stack, print_trace, f);
	fprintf(f, "\"\" ]\n");
	fprintf(f, "\t\t}");
}
#endif

static void
save_mprofile_record(FILE *f, struct mprofile_record *mpr)
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
	fprintf(f, "\t\t{\n");
	fprintf(f, "\t\t\t%s : 0x%p,\n", MPROFILE_REC_MEM, mpr->mpr_mem);
	fprintf(f, "\t\t\t%s : 0x%p,\n", MPROFILE_REC_REALLOC,
	    mpr->mpr_realloc);
	fprintf(f, "\t\t\t%s : %z,\n", MPROFILE_REC_SZ, mpr->mpr_sz);
	fprintf(f, "\t\t\t%s : %s,\n", MPROFILE_REC_STATE, state);
	fprintf(f, "\t\t\t%s : {\n", MPROFILE_TIMESTAMP);
	fprintf(f, "\t\t\t\t%s : %lu,\n", MPROFILE_TIME_S, mpr->mpr_ts.tv_sec);
	fprintf(f, "\t\t\t\t%s : %lu,\n", MPROFILE_TIME_NS,
	    mpr->mpr_ts.tv_nsec);
	fprintf(f, "\t\t\t}\n");
	fprintf(f, "\t\t},\n");
}

static void
save_mprofile_entry(FILE *f, struct mprofile_entry *mpe)
{
	struct mprofile_record_set *mrs;
	struct mprofile_record *mpr;
#ifdef _WITH_STACKTRACE
	mprofile_stack_t *mps;
#endif

	if (RB_EMPTY(&mpe->mpe_rbh))
		return;

	RB_FOREACH(mrs, mprofile_record_set, &mpe->mpe_rbh) {
		fprintf(f, "\t{\n");
		fprintf(f, "\t\t%s : %z\n", MPROFILE_RS_SZ, mrs->mrs_sz);
#ifdef _WITH_STACKTRACE
		mps = mprofile_get_next_stack(mrs->mrs_stset, NULL);
		fprintf(f, "\t\t%s : [\n", MPROFILE_RS_STACKS);
		while (mps != NULL) {
			mprofile_print_stack(f, mps);
			mps = mprofile_get_next_stack(mrs->mrs_stset, mps);
			if (mrs != NULL) {
				print_stack(f, mps);
				fprintf(",\n");
			}
		}
		fprintf(f, "\t\t],\n");
#endif
		fprintf(f, "\t\t%s : [\n", MPROFILE_RS_BUFLIST);
		LIST_FOREACH(mpr, &mre->mrs_lhead, mpr_le)
			save_mprofile_record(f, mpr);
		fprintf(f, "\t\t],\n");
		fprintf(f, "\t},\n");
	}
}

mprofile_t *
mprofile_init(void)
{
	mprofile_t *mp;
	unsigned int i;
	/* the sizez here are pure guess work */
	static size_t sizes[MPROFILE_COUNT] = {
		0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, /* 10 */
		0x2000, 0x4000, 0x8000, 0x10400, 0x10800, 0x11000, 0x12000, 0x14000, /* 8 */
		0x18000, 0x20000, 0x20100, 0x20200, 0x20400, 0x20800, 0x21000, 0x21200, /* 8 */
		0x21400, 0x21800, 0x40000, 0x80000, 0x80400, 0x0 };
	};

	mp = (mprofile_t *) malloc(sizeof (mprofile_t));
	if (mp == NULL)
		return (NULL);

	mp->mp_cnt = MPROFILE_COUNT;
	for (i = 0; i < MPROFILE_COUNT; i++) {
		RB_INIT(&mp->mp_entries[i].mpe_rbh);
		mp->mp_entries[i].mpe_max_sz = sizes[i];
	}

	return (mp);
}

void
mprofile_save(mprofile_t *mp)
{
	unsigned int i;
	char *fname = getenv("MPROFILE_OUTF");
	FILE *f;

	if (fname == NULL)
		return;

	f = fopen(fname, "w");
	if (f == NULL)
		return;

	fprintf(f, "[\n");
	for (i = 0; i < mp->mp_cnt, i++) {
		save_mprofile_entry(f, &mp->mp_entries[i]);
	}
	fprintf(f, "]");

	fclose(f);
}

void
mprofile_destroy(mprofile_t *mp)
{
	unsigned int i;

	for (i = 0; i < mp->mp_cnt; i++)
		destroy_entry(&mp->mp_entries[i]);

	free(mp);
}

void
mprofile_record_alloc(mprofile_t *mp, void *buf, size_t buf_sz, mprofile_stack_t *mps)
{
	struct mprofile_entry *mpe = find_entry_for_size(buf_sz);
	struct mprofile_record_set *mrs = find_record_set_for_size(mpe, buf_sz);
	struct mprofile_record mpr;

	if (mrs == NULL) {
		mrs = create_record_set_for_size(mpe, buf_sz);
		if (mrs == NULL)
			return;
	}

	mpr = create_record();
	mpr->mpr_mem = buf;
	mpr->mpr_sz = buf_sz;
	mpr->mpr_state = ALLOC;
	LIST_INSERT_HEAD(&mrs->mrs_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mprofile_add_stack(mrs->mrs_stset, mps);
#endif
}

void
mprofile_record_free(mprofile_t *mp, void *buf, size_t buf_sz, mprofile_stack_t *mps)
{
	struct mprofile_entry *mpe = find_entry_for_size(buf_sz);
	struct mprofile_record_set *mrs = find_record_set_for_size(mpe, buf_sz);
	struct mprofile_record mpr;

	if (mrs == NULL) {
		mrs = create_record_set_for_size(mpe, buf_sz);
		if (mrs == NULL)
			return;
	}

	mpr = create_record();
	mpr->mpr_mem = buf;
	mpr->mpr_sz = buf_sz;
	mpr->mpr_state = FREE;
	LIST_INSERT_HEAD(&mrs->mrs_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mprofile_add_stack(mrs->mrs_stset, mps);
#endif
}

void
mprofile_record_realloc(mprofile_t *mp, void *buf, size_t buf_sz,
    void *old_buf, mprofile_stack_t *mps)
{
	struct mprofile_entry *mpe = find_entry_for_size(buf_sz);
	struct mprofile_record_set *mrs = find_record_set_for_size(mpe, buf_sz);
	struct mprofile_record mpr;

	if (mrs == NULL) {
		mrs = create_record_set_for_size(mpe, buf_sz);
		if (mrs == NULL)
			return;
	}

	mpr = create_record();
	mpr->mpr_mem = buf;
	mpr->mpr_sz = buf_sz;
	mpr->mpr_realloc = old_buf;
	mpr->mpr_state = REALLOC;
	LIST_INSERT_HEAD(&mrs->mrs_lhead, mpr, mpr_le);

#ifdef _WITH_STACKTRACE
	mprofile_add_stack(mrs->mrs_stset, mps);
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
walks_stack(unsigned long long frame, void *arg)
{
	DL_info dli;

	if (dladdr((void *)frame, &dli != 0)
		add_shlib(dli);
}

/*
 * This is expensive, we walk trhough all stacks
 * to find list of shared libraries. There must
 * better way to obtain the list of shared libraries.
 */
static void
visit_stacks(struct mprofile_entry *mpe)
{
	struct mprofile_record_set *mrs;
	mprofile_stack_t *stack;

	RB_FOREACH(mrs, mprofile_record_set, &mpe->mpe_rbh) {
		stack = mprofile_get_next_stack(mrs->mrs_stset, NULL);
		while (stack != NULL) {
			mprofile_walk_stack(mps, walk_stack, NULL);
			stack = mprofile_get_next_stack(mrs->mrs_stset, stack);
		}
	}
}

void
mprofile_load_syms(mprofile_t *mp)
{
	unsigned int	i;

	for (i = 0; i < MPROFILE_COUNT; i++) 
		visit_stacks(&mp->mp_entries[i])

	for (i = 0; i < MAX_SHLIBS; i++) {
		if (shlibs[i].shl_name != NULL)
			syms = kelf_open(shlibs[i].shl_name, syms, shlibs[i].shl_base);
	}
}

void
mprofile_done(void)
{
	unsigned int	i;

	for (i = 0; i < MAX_SHLIBS; i++)
		free(shlibs[i].shl_name);

	 kelf_close(syms);
}
