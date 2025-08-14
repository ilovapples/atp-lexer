#include "lexer.h"
#include "preproc.h"
#include "util.h"

extern char *SRC_PATH_L;
extern u32 errflags;

s32 main(s32 argc, char **argv)
{
	parse_args_preproc(argc, argv, print_usage_msg_lexer, &SRC_PATH_L, NULL);

	struct str_buf src_contents = read_file_to_string(SRC_PATH_L);
	struct str_buf file_contents = strip_comments(src_contents, SRC_PATH_L);
	free(src_contents.buf);

	lexer_init(file_contents);

	Token cur_token;
	while (!is_null_token(cur_token = next_token()))
	{
		if (errflags != 0)
		{
			flogf(LOG_ERR, stderr, "error encountered; terminating token stream...\n");
			exit(1);
		}
		struct str_buf esc_str = dbg_escape_str(cur_token.value);
		printf("{ type: 0x%02X, subtype: 0x%02X, value: \"%.*s\" }\n",
			 cur_token.type,
			 cur_token.subtype,
			 (int)esc_str.len,
			 esc_str.buf);
	}
	freetmp();

	return 0;
}
