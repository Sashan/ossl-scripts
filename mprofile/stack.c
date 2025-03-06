#include <stdlib.h>

#include "utils/tree.h"

#define	MPS_STACK_DEPTH		64

#define	MPS_FLAG_MANAGED	1

struct mprofile_stack_managed {
};

struct mprofile_stack {
	size_t				 mps_stack_limit;
	unsigned int			 mps_stack_id;
	unsigned int			 mps_stack_depth;
	unsigned int			 mps_flags;
	unsigned int			 mps_count;
	pthread_t			 mps_thread;
	RB_ENTRY(mprofile_stack_managed) mps_rbe;
	unsigned long long		*mps_stack;
};

struct mprofile_stack_set {
	RB_HEAD(mp_stack_set, mprofile_stack)	stset_rbh;
};

static int stack_compare(mprofile_stack_t *, mprofile_stack_t *);

RB_GENERATE_STATIC(mprofile_stack_set, mprofile_stack, mps_rbe, stack_compare);

static int
stack_compare(mprofile_stack_t *a_mps, mprofile_stack_t *b_mps)
{
	int	i = 0;

	if (a_mps->mps_stack_depth < b_mps->mps_stack_depth)
		return (-1);
	else if (a_mps->mps_stack_depth > b_mps->mps_stack_depth)
		return (1);
	else while ((a_mps->mps_stack[i] == b_mps->mps_stack[i]) &&
	    (i < b_mps->mps_stack_depth))
		i++;

	if (a_mps->mps_stack[i] < b_mps->mps_stack_depth)
		return (-1);
	else if (a_mps->mps_stack[i] > b_mps->mps_stack_depth[i])
		return (1);

	return (0);
}

mprofile_stack_t *
mprofile_init_stack(void *buf, size_t buf_sz)
{
	mprofile_stack_t *mps;
	size_t		  stack_sz;

	if (buf == NULL) {
		stack_sz =
		    sizeof (struct mprofile_stack_set) +
		    sizeof (unsigned long long) * (MPS_STACK_DEPTH - 1);
		mps = (mprofile_stack_t *) malloc(stack_sz);
		if (mps == NULL)
			return (NULL);
		mps->mps_flags = MPS_FLAG_MANAGED;
		mps->mps_stack_depth = 0;
		mps->mps_stack_limit = MPS_STACK_DEPTH;
		memset(mps->mps_stack, 0,
		    sizeof (unsigned long long) * MPS_STACK_DEPTH);
		mps->mps_thread = 0;	/* TODO: set thread id/pthread_t */
		mps->mps_count = 0;
	} else {
		if (buf_sz <
		    (sizeof (mprofile_stack_t) + (sizeof (unsigned long long))
			return NULL;
		memset(buf, 0, buf_sz);
		mps = (mprofile_stack_t *)buf;
		stack_sz = buf_sz - sizeof (mprofile_stack_t);
		stack_sz += sizeof (unsigned long long);
		mps->mps_stack_limit = stack_sz / sizeof (unsigned long long);
		mps->mps_stack_depth = 0;
	}

	return (mps);
}

mprofile_stack_t *
mprofile_copy_stack(mprofile_stack_t *mps)
{
	mprofile_stack_t *new_mps;
	unsigned int i;

	new_mps = (mprofile_stack_t *) malloc(
		sizeof (struct mprofile_stack_managed) +
		sizeof (unsigned long long) * mps->mps_stack_depth);
	if (new_mps == NULL)
		return (NULL);

	for (i = 0; i < mps->mps_stack_depth; i++)
		new_mps->mps_stack[i] = mps->mps_stack[i];

	new_mps->mps_stack_depth = mps->mps_stack_depth;
	/* using stack_depth here freezes the stack */
	new_mps->mps_stack_limit = mps->mps_stack_depth;
	new_mps->mps_flags = MPS_FLAG_MANAGED;
	if (mps->mps_flags == MPS_FLAG_MANAGED) {
		new_mps->mps_count = mps->mps_count;
		new_mps->mps_thread = mps->mps_thread;
	} else {
		new_mps->mps_count = 1;
		new_mps->mps_thread = 0; /* get thread id */
	}

	return (msm_mps);
}

mprofile_stack_t *
mprofile_add_stack(mprofile_stset_t *stset. mprofile_stack_t *mps)
{
	struct mprofile_stack_managed	*found_mps;
	static unsigned int		 stack_id = 1;

	if (stset == NULL)
		return (NULL);

	found_mps = RB_FIND(mp_stack_set, &stset->stset_rbh, mps);
	if (found_mps) {
		found_mps->mps_count++;
	} else {
		found_mps = mprofile_copy_stack(mps);
		if (found_mps == NULL)
			return (NULL);
		found_mps->mps_stack_id = stack_id++;
		RB_INSERT(mp_stack_set, &stset->stset_rbh, found_mps);
	}

	return (found_mps);
}

void
mprofile_destroy_stack(mprofile_stack_t *mps)
{
	assert(mps->mps_flags == MPS_FLAG_MANAGED);

	free(mps->mps_msm);
}

void
mprofile_push_frame(mprofile_stack_t *mps, unsigned long long frame)
{
	if (mps == NULL)
		return;

	if (mps->mps_stack_depth < mps->mps_stack_limit) {
		mps->mps_stack[mps->mps_stack_depth] = frame;
		mps->mps_stack_depth++;
	}
}

mprofile_stack_set_t *
mprofile_create_stset(void)
{
	mprofile_stack_set_t *stset;

	stset = (mprofile_stack_set_t *) malloc(sizeof (mprofile_stack_set_t));
	if (stset != NULL)
		RB_INIT(stset->stset_rbh);

	return (stset);
}

void
mprofile_destroy_stset(mprofile_stset_t *stset)
{
	struct mprofile_stack_managed *msm, *walk;

	if (stset == NULL)
		return;

	RB_FOREACH_SAFE(msm, mprofile_stack_managed, &stset->stset_rbh, walk) {
		RB_REMOVE(mprofile_stack_managed, &stset->stset_rbh, msm);
		mprofile_destroy_stack(msm->msm_mps);
	}

	free(stset);
}

mprofile_stack_t *
mprofile_get_next_stack(mprofile_stset_t *stset, mprofile_stack_t *mps)
{
	if (stset == NULL)
		return (NULL);

	if (mps == NULL)
		return (RB_MIN(mprofile_stset, &stset->stset_rbh));

	return (RB_NEXT(mprofile_stset, mps));
}

void
mprofile_walk_stack(mprofile_stack_t *mps,
    (void)(*walk_f)(unsigned long long, void *), void *walk_arg)
{
	unsigned int i;

	for (i = 0; i < mps->mps_stack_depth; i++)
		walk_f(mps->mps_stack[i], walk_arg);
}

unsigned int
mprofile_get_stack_id(mprofile_stack_t *mps)
{
	if (mps == NULL)
		return (0);

	return (mps->mps_stack_id);
}

unsigned int
mprofile_get_stack_count(mprofile_stack_t *mps)
{
	if (mps == NULL)
		return (0);

	return (mps->mps_count);
}
