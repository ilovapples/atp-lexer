#ifndef PREPROC_H
#define PREPROC_H

#include "types.h"
#include "util.h"

void print_usage_msg_preproc(void);
void parse_args_preproc(s32 argc, char **argv, void (*usage_msg)(void),
		char **src_path, char **dst_path);
struct str_buf strip_comments(struct str_buf in_buf, char *container_filename);

#endif /* PREPROC_H */
