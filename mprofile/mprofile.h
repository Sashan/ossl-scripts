#ifndef _MPROFILE_H_
#define	_MPROFILE_H_
#include <stdio.h>

typedef struct mprofile_stack mprofile_stack_t;
typedef struct mprofile_stack_set mprofile_stset_t;
typedef struct mprofile_t;

#ifdef _WITH_STACKTRACE
mprofile_stack_set_t *mprofile_create_stset();
void mprofile_destroy_stset(mprofile_stset_t *);
void mprofile_add_stack(mprofile_stset_t *, mprofile_stack_t *);
mprofile_stack_t *mprofile_get_next_stack(mprofile_stset_t *,
    mprofile_stack_t *);
mprofile_stack_t *mprofile_init_stack(unsigned long long *, size_t);
mprofile_stack_t *mprofile_copy_stack(mprofile_stack_t *);
void mprofile_walk_stack(mprofile_stset_t *,
    (void)(*)(unsigned long long, void *), void *);
void mprofile_destroy_stack(mprofile_stack_t *);
void mprofile_push_frame(mprofile_stack_t *, unsigned long long);
unsigned int mprofile_get_stack_count(mprofile_stack_t *);
unsigned int mprofile_get_stack_id(mprofile_stack_t *);
#endif

void mprofile_record_alloc(mprpfile_t *, void *, size_t, mprofile_stack_t *);
void mprofile_record_free(mprofile_t *,void *, mprofile_stack_t *);
void mprofile_record_realloc(mprofile_t, void *, size_t, void *,
    mprofile_stack_t *);

mprofile_t *mprofile_create(void);
void mprofile_load_syms(mprofile_t *);
void mprofile_save(mprofile_t *);
void mprofile_destroy(mprofile_t *);
void mprofile_done(void);
#endif
