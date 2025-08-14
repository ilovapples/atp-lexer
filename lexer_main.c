#include "lexer.h"
#include "preproc.h"
#include "util.h"

#include <string.h>
#include <stdio.h>

extern char *SRC_PATH_L;
extern u32 errflags;

s32 main(s32 argc, char **argv)
{
	parse_args_preproc(argc, argv, print_usage_msg_lexer, &SRC_PATH_L, NULL);

	struct str_buf src_contents = read_file_to_string(SRC_PATH_L);
#ifdef STRIP_COMMENTS
	struct str_buf file_contents = strip_comments(src_contents, SRC_PATH_L);
	free(src_contents.buf);

	lexer_init(file_contents);
#else
	lexer_init(src_contents);
#endif
	Token cur_token;
	while (!is_null_token(cur_token = next_token()))
	{
		if (geterr(INT_LITERAL_HAS_NO_VALID_DIGITS))
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

#ifdef STRIP_COMMENTS
	free(file_contents.buf);
#else
	free(src_contents.buf);
#endif

	return 0;
}
