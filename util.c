#include "util.h"

#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#ifndef BARE_UTIL_FLAG
#include "args.h"
#endif

char *cpybuftostr(char *dst_str, strbuf src_buf)
{
	return strncpy(dst_str, src_buf.buf, src_buf.len);
}

char *cpybuftobuf(strbuf *dst_buf, strbuf src_buf)
{
	dst_buf->len = MAX(MIN(src_buf.len, dst_buf->capacity), dst_buf->len);
	// cap > dlen > slen = dlen;
	// slen > capacity > dlen = cap;
	return strncpy(dst_buf->buf, src_buf.buf, MIN(dst_buf->capacity, src_buf.len));
}

void error(s32 code, const char *fmt, ...)
{
	va_list arg_list;
	va_start(arg_list, fmt);
	vfprintf(stderr, fmt, arg_list);
	va_end(arg_list);
	if (code > -5280)
		exit(code);
}

#ifndef BARE_UTIL_FLAG
void vflogf(LOG_TYPE type, FILE *stream, const char *fmt, va_list arg_list)
{
	switch (type){
	case LOG_ERR:
		if (ISCLR)
			fprintf(stream, ERR_LOG);
		else
			fprintf(stream, ERR_ENT);
		break;
	case LOG_WARN:
		if (ISCLR)
			fprintf(stream, WARN_LOG);
		else
			fprintf(stream, WARN_ENT);
		break;
	case LOG_INFO:
		if (ISCLR)
			fprintf(stream, INFO_LOG);
		else
			fprintf(stream, INFO_ENT);
		break;
	case LOG_DEBUG:
		if (!FLAG_SET(DEBUG))
			return;
		if (ISCLR)
			fprintf(stream, DEBUG_LOG);
		else
			fprintf(stream, DEBUG_ENT);
		break;
	}

	vfprintf(stream, fmt, arg_list);
	if (ISCLR) fprintf(stream, LOG_END);
}

void flogf(LOG_TYPE type, FILE *stream, const char *fmt, ...)
{
	va_list arg_list;
	va_start(arg_list, fmt);
	vflogf(type, stream, fmt, arg_list);
	va_end(arg_list);
}

void print_info(void)
{
	printf("Compiled on %s at %s with GCC %s, C standard version %ld.\n", 
			__DATE__, __TIME__, __VERSION__, __STDC_VERSION__);
	exit(0);
}

strbuf read_file_to_string(const char *file_name)
{
	FILE *fp = fopen(file_name, "rb");
	if (fp == NULL)
	{
		flogf(LOG_ERR, stderr, "failed to open file '%s'\n", file_name);
		exit(2);
	}

	size_t buf_size = 2048;
	strbuf ret_buf = {0};
	ret_buf.buf = malloc(buf_size);
	if (ret_buf.buf == NULL)
	{
		fclose(fp);
		flogf(LOG_ERR, stderr, "failed to allocate the initial buffer size\n");
		exit(3);
	}

	size_t cur_chunk_read;
	
	while ((cur_chunk_read = fread(ret_buf.buf + ret_buf.len, 1, buf_size - ret_buf.len - 1, fp)) > 0)
	{
		ret_buf.len += cur_chunk_read;
		if (ret_buf.len >= buf_size - 1)
		{
			buf_size *= 2;
			char *temp_buf = realloc(ret_buf.buf, buf_size);
			if (temp_buf == NULL)
			{
				free(ret_buf.buf);
				fclose(fp);
				flogf(LOG_ERR, stderr, "failed to reallocate buffer with size %zu\n", buf_size);
				exit(4);
			}
			ret_buf.buf = temp_buf;
		}
	}

	ret_buf.buf[ret_buf.len++] = '\0';

	fclose(fp);
	return ret_buf;
}

s32 write_buf_to_file(strbuf buf, const char *dst_path)
{
	// check if the file exists
	if (!FLAG_SET(FORCE_OVERWRITE) && dst_path != NULL)
	{
		bool file_exists = true;
		FILE *ftmp;
		if ((ftmp = fopen(dst_path, "rb")) == NULL)
			file_exists = !(errno == ENOENT);
		else
			fclose(ftmp);
		if (file_exists)
		{
			printf("File at path '%s' exists. Overwrite? (Y/n): ", dst_path);
			char reply_buf[2];
			fgets(reply_buf, 2, stdin);
			if (tolower(reply_buf[0]) == 'n')
				return 2; // quit due to user override
		}
	}

	if (dst_path == NULL)
	{
		fwrite(buf.buf, 1, buf.len, stdout);
		return 1; // wrote to stdout
	}

	FILE *fp = fopen(dst_path, "wb");
	if (fp == NULL)
		return -1; // failed to open destination file

	fwrite(buf.buf, 1, buf.len, fp);

	fclose(fp);

	return 0;
}

extern u8 tab_width;

void debug_print_pos(FILE *stream, struct str_buf substr, char *container, size_t line_num, 
		u8 highlight_color, u8 caret_color, LOG_TYPE log_type, const char *msg_fmt, ...)
{
		if (log_type == LOG_DEBUG && !FLAG_SET(DEBUG))
			return;

		char *prev_newline = substr.buf;
		while (prev_newline > container && *(prev_newline-1) != '\n')
			prev_newline--;

		char *next_newline = substr.buf;
		while (*next_newline && *next_newline != '\n')
			next_newline++;

		size_t line_len = next_newline - prev_newline;
		size_t caret_pos = 0;
		size_t n_tabs = 0;
		for (char *c = prev_newline; c < substr.buf; ++c)
		{
			if (*c == '\t')
			{
				caret_pos += tab_width - (caret_pos % tab_width);
				n_tabs++;
			} else
				caret_pos++;
		}
		
		size_t col_n = substr.buf - prev_newline + 1;

		va_list arg_list;
		va_start(arg_list, msg_fmt);
		vflogf(log_type, stream, msg_fmt, arg_list);
		va_end(arg_list);

		if (ISCLR)
		{
			if (col_n != caret_pos+1)
				fprintf(stream, "\x1b[38;5;242m --> %s:%zu;%zu-%zu\n\x1b[0m",
						substr.container_filename, line_num, col_n, caret_pos + 1);
			else
				fprintf(stream, "\x1b[38;5;242m --> %s:%zu;%zu\n\x1b[0m",
						substr.container_filename, line_num, col_n);
		} else
			fprintf(stream, " --> %s:%zu;%zu\n",
					substr.container_filename, line_num, col_n);

		char *tildes_buf = malloc(substr.len-1 + 1); // -1 to exclude ^, +1 for '\0'
		memset(tildes_buf, '~', substr.len-1);
		tildes_buf[substr.len-1] = '\0';

		if (!ISCLR)
		{
			fprintf(stream, "%5zu | %.*s\n"
					    "%*s^%s\n",
				line_num, (int) line_len, prev_newline,
				(int) caret_pos, "", tildes_buf);

			free(tildes_buf);

			return;
		}

		// 15 is the max byte len of the escape construction for the color
		char *buf = calloc(line_len+1+15+tab_width*n_tabs, 1);
		char *bufp = buf;
		char *c = prev_newline;
		while (c < substr.buf)
		{
			if (*c == '\t') {
				for (u8 i = 0; i < tab_width; ++i)
					*bufp++ = ' ';
				c++;
			} else
				*bufp++ = *c++;
		}
		bufp += snprintf(bufp, 12, "\x1b[38;5;%um", highlight_color);
		

		for (size_t i = 0; i < substr.len; ++i)
			*bufp++ = *c++;
		strncpy(bufp, "\x1b[0m", 4);
		bufp += 4;
		while (c < next_newline)
			*bufp++ = *c++;

		#define NUM_COLOR "248"
		fprintf(stream, SET_FG_ESC NUM_COLOR "m%4zu | "LOG_END"%s\n"
				    SET_FG_ESC NUM_COLOR "m     | "
				    LOG_END"%*s"SET_FG_ESC"%um^%s"LOG_END"\n\n",
			line_num, buf,
			(int) caret_pos, "", caret_color, tildes_buf);

		free(tildes_buf);
		free(buf);
}
#endif

#define ESC_CHAR_SIZE 4
bool temp_str_is_freed = false;

static char *temp_str = NULL;

struct str_buf dbg_escape_str(struct str_buf str)
{
	if (!temp_str_is_freed) {
		free(temp_str);
		temp_str_is_freed = true;
	}

	size_t num_escaped = 0;
	char *strbufpos = str.buf;
	while (strbufpos < str.buf + str.len)
	{
		if (iscntrl(*strbufpos))
			num_escaped++;
		strbufpos++;
	}
	const size_t retval_len = ESC_CHAR_SIZE*num_escaped + (str.len - num_escaped) + 1;

	temp_str = malloc(retval_len);
	char *retval_pos = temp_str;
	const char *buf_start = str.buf;
	while (retval_pos < temp_str + retval_len && str.buf < buf_start + str.len) {
		retval_pos += chresc(*str.buf, retval_pos);
		str.buf++;
	}

	return strbuflit(temp_str, retval_len - 1, NULL);
}

size_t chresc(char c, char *out)
{
	if (!iscntrl(c)) {
		out[0] = c;
		return 1;
	} else
		return snprintf(out, ESC_CHAR_SIZE+1, "<%02x>", c);
}

void freetmp(void)
{
	if (!temp_str_is_freed) {
		free(temp_str);
		temp_str_is_freed = true;
	}
}
