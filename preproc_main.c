#include <string.h>

#include "types.h"
#include "preproc.h"

extern char *SRC_PATH_P, *DST_PATH_P;


s32 main(s32 argc, char **argv)
{
	parse_args_preproc(argc, argv, print_usage_msg_preproc, &SRC_PATH_P, &DST_PATH_P);

	struct str_buf in_buf = read_file_to_string(SRC_PATH_P);
	struct str_buf out_buf = strip_comments(in_buf, SRC_PATH_P);
	free(in_buf.buf);

	if (DST_PATH_P && strcmp(DST_PATH_P, "nope") == 0) {
		free(out_buf.buf);
		return 0;
	}

	s32 err;
	if ((err = write_buf_to_file(out_buf, DST_PATH_P)) < 0) {
		flogf(LOG_ERR, stderr, "failed to open destination file '%s' for writing.\n", DST_PATH_P);
		exit(err);
	}
		

	free(out_buf.buf);

	return 0;
}
