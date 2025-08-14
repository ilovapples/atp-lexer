#ifndef ARGS_H
#define ARGS_H

#include "types.h"

extern char *PROG_NAME;
extern u8 runtime_flags;

#define BIT(n) (1<<n)
#define SET_FLAG(f) (runtime_flags |= f)
#define FLAG_SET(f) ((runtime_flags & f) != 0)
#define ISCLR (!FLAG_SET(NO_COLOR))

enum {
	FORCE_OVERWRITE = BIT(0),
	NO_COLOR = BIT(1),
	DEBUG = BIT(2),
};

#endif /* ARGS_H */
