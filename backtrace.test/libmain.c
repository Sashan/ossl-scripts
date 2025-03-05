#include <stdlib.h>

#include "backtrac.h"

/*
 * We need to fire at exit, so all shared libraries are still loaded,
 * so we will be able to resolve symbols.
 */
static void
on_exit(void)
{
	bt_print_staack();
}

void
_init(void)
{
	bt_init();
	atexit(on_exit);
}

void
_fini(void)
{
	bt_done();
}
