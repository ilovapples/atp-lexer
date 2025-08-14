#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "util.h"
#include "args.h"

char *SRC_PATH_P = NULL, *DST_PATH_P = NULL;

u8 tab_width = 6;

void print_usage_msg_preproc(void)
{
	error(1, "usage: %s [options] <in_file>\n\n"

		   "  -o OUT_FILE       specifies that the output is written to OUT_FILE\n"
		   "  -f, --force       disables asking whether to overwrite an existing output file\n"
		   "  --tab-width=N     sets tab display width to N cells (no effect with --no-color for reasons)\n"
		   "  -h, --help        show this help message\n"
		   "  -d, --debug       enable debug output\n"
		   "  --no-color        print output without color\n"
		   "  --info            print program info\n"
			, PROG_NAME);
}

void parse_args_preproc(s32 argc, char **argv, void (*usage_msg) (void),
		char **src_path, char **dst_path
) {
	PROG_NAME = argv[0];
	for (s32 arg_n = 1; arg_n < argc; ++arg_n) {
		if (argv[arg_n][0] == '-') {
			if (argv[arg_n][1] == '-') {
				if (strcmp(argv[arg_n]+2, "no-color") == 0)
					SET_FLAG(NO_COLOR);
				else if (strcmp(argv[arg_n]+2, "force") == 0)
					SET_FLAG(FORCE_OVERWRITE);
				else if (strncmp(argv[arg_n]+2, "tab-width=", 10) == 0)
					tab_width = strtoul(argv[arg_n]+2+10, NULL, 10);
				else if (strcmp(argv[arg_n]+2, "help") == 0)
					(*usage_msg)();
				else if (strcmp(argv[arg_n]+2, "info") == 0)
					print_info();
				else if (strcmp(argv[arg_n]+2, "debug") == 0)
					SET_FLAG(DEBUG);
				else
					flogf(LOG_ERR, stderr, "Unknown option '%s'\n", argv[arg_n]);
			} else {
				char *cur_chr = argv[arg_n];
				bool will_terminate = false;
				while (*(++cur_chr) && !will_terminate) {
					switch (*cur_chr) {
					case 'h':
						(*usage_msg)();
						break;
					case 'f':
						SET_FLAG(FORCE_OVERWRITE);
						break;
					case 'd':
						SET_FLAG(DEBUG);
						break;
					case 'o':
						if (arg_n + 1 < argc && dst_path != NULL)
							*dst_path = argv[++arg_n];
						else
							(*usage_msg)();
						will_terminate = true;
						break;
					default:
						flogf(LOG_ERR, stderr, "Unknown option '-%c'\n", *cur_chr);
					}
				}
			}
		} else if (*src_path == NULL)
			*src_path = argv[arg_n];
	}

	if (*src_path == NULL)
		(*usage_msg)();
}

struct str_buf strip_comments(struct str_buf in_buf, char *container_filename)
{
	struct str_buf out_file = {0};
	out_file.buf = calloc(in_buf.len, 1);
	out_file.len = 0;

	bool in_short_comment = false;
	bool in_comment;

	char *cur_comment_start = NULL;
	size_t cur_comment_start_line_n = 0;

	size_t n_dquotes = 0;
	size_t n_squotes = 0;
#define IN_STRING() (n_dquotes % 2 == 1)
#define IN_CHAR() (n_squotes %2 == 1)
	size_t n_consec_backslashes = 0;
#define IS_ESCAPED() (n_consec_backslashes % 2 == 1)

	char *ch = in_buf.buf;
	char *outch = out_file.buf;
	size_t line_n = 1;
	/* strip comments from the file at SRC_PATH and output it into DST_PATH */
	while (*ch) {
		if (*ch == '\\')
		{
			n_consec_backslashes++;
		} else
			n_consec_backslashes = 0;


		if (*ch == '"' && !IS_ESCAPED() && !IN_CHAR())
			n_dquotes++;
		if (*ch == '\'' && !IS_ESCAPED() && !IN_STRING()) {
			n_squotes++;
		}

		// long comment start
		if (!in_comment && !IN_STRING() && (strncmp(ch, "/*", 2) == 0)) {
			cur_comment_start = ch;
			/*if (FLAG_SET(DEBUG))
				debug_print_pos(stdout, strbuflit(cur_comment_start, 2, container_filename),
					in_buf.buf, line_n, 
					BLAND_CARET_COLOR, BLAND_CARET_COLOR,
					LOG_INFO, "Long comment starts near line %zu.\n",
					line_n);*/
			cur_comment_start_line_n = line_n;
			ch += 2;
		}
		// long comment end
		if (cur_comment_start != NULL && !IN_STRING() && (strncmp(ch, "*/", 2) == 0)) {
			cur_comment_start = NULL;
			cur_comment_start_line_n = 0;
			ch += 2;
		}

		// short comment start
		if (!in_comment && !IN_STRING() && (strncmp(ch, "//", 2) == 0)) {
			in_short_comment = true;
			/*if (FLAG_SET(DEBUG))
				debug_print_pos(stdout, strbuflit(ch, 2, container_filename),
						in_buf.buf, line_n,
						BLAND_CARET_COLOR, BLAND_CARET_COLOR,
						LOG_INFO, "Short comment starts near line %zu.\n",
						line_n);*/
			ch += 2;
		}

		// windows line-ending
		if (*ch == '\r')
			ch++;
		if (*ch == '\n')
			line_n++;
		// short comment end
		if (in_comment && !IN_STRING() && *ch == '\n')
			in_short_comment = false;

		in_comment = (cur_comment_start != NULL || in_short_comment);

		if (!in_comment) {
			*outch++ = *ch;
			out_file.len++;
		}

		ch++;
	}

	line_n--;

	if (cur_comment_start != NULL) {
		debug_print_pos(stderr, strbuflit(cur_comment_start, 2, container_filename), in_buf.buf,
			cur_comment_start_line_n, ERR_COLOR, ERR_COLOR,
			LOG_ERR, "unterminated comment:\n",
			cur_comment_start_line_n);
		exit(6);
	}

	return out_file;
}
