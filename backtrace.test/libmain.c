#include <stdlib.h>

#include "backtrace.h"

static void __attribute__ ((constructor)) init(void);
static void __attribute__ ((destructor)) done(void);
/*
 * We need to fire at exit, so all shared libraries are still loaded,
 * so we will be able to resolve symbols.
 */
static void
print_stack(void)
{
	bt_print_stack();
}

static void
init(void)
{
	bt_init();
	atexit(print_stack);
}

static void
done(void)
{
	bt_done();
}
