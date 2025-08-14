#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#define SET_FG_ESC "\x1b[38;5;"
#define ERR_COLOR 9
#define WARN_COLOR 214
#define INFO_COLOR 81
#define DEBUG_COLOR 255
#define ERR_COLOR_STR "9"
#define WARN_COLOR_STR "214"
#define INFO_COLOR_STR "81"
#define DEBUG_COLOR_STR "255"
#define ERR_ENT "[ERROR] "
#define WARN_ENT "[WARN] "
#define INFO_ENT "[INFO] "
#define DEBUG_ENT "[DEBUG] "
#define ERR_LOG SET_FG_ESC ERR_COLOR_STR ";1m" ERR_ENT "\x1b[0m" SET_FG_ESC ERR_COLOR_STR "m"
#define WARN_LOG SET_FG_ESC WARN_COLOR_STR ";1m" WARN_ENT "\x1b[0m" SET_FG_ESC WARN_COLOR_STR "m"
#define INFO_LOG SET_FG_ESC INFO_COLOR_STR ";1m" INFO_ENT "\x1b[0m" SET_FG_ESC INFO_COLOR_STR "m"
#define DEBUG_LOG SET_FG_ESC DEBUG_COLOR_STR ";1m" DEBUG_ENT "\x1b[0m" SET_FG_ESC DEBUG_COLOR_STR "m"
#define LOG_END "\x1b[0m"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define strbuflit(chp, len, file) ((struct str_buf) { (chp), (file), (len), (len) })

typedef struct str_buf {
	char *buf;
	char *container_filename;
	size_t len;
	size_t capacity;
} strbuf;

char *cpybuftostr(char *dst_str, strbuf src_buf);
char *cpybuftobuf(strbuf *dst_buf, strbuf src_buf);

typedef enum {
	LOG_ERR,
	LOG_WARN,
	LOG_INFO,
	LOG_DEBUG,
} LOG_TYPE;

/* prints `fmt` with subsequent arguments to stderr
 * via `printf` formatting, and calls `exit(code);` 
 * (unless `code` <= -5280).
 */
void error(s32 code, const char *fmt, ...);

/* Logs to `stream` with `fmt` and subsequent arguments via the
 * `printf` format, prefixed and colored depending on `type`.
 */
void vflogf(LOG_TYPE type, FILE *stream, const char *fmt, va_list arg_list); 
void flogf(LOG_TYPE type, FILE *stream, const char *fmt, ...);

/* Prints program info (date of compilation, compiler version, etc.).
 */
void print_info(void);

/* Reads the file at `file_name` into a `struct str_buf` and returns it.
 * Allocated size of the returned buffer will be a power of two, while the
 * len stored in the return value will be the total number of bytes 
 * read from `file_name`.
 */
struct str_buf read_file_to_string(const char *file_name);

/* Writes the contents of `out_buf` (don't question naming) to 
 * the file at `dst_path`. Returns a value >= 0 if successful:
 * 0 if the buffer was succesfully written to the file.
 * 1 if the buffer was printed to stdout.
 * 2 if the user was prompted to confirm overwriting an existing file chose not to.
 * -1 if `dst_path` could not be opened for writing.
 */
s32 write_buf_to_file(struct str_buf buf, const char *dst_path);

/* You don't get documentation for this function because I don't feel
 * like writing it. This project is open source; read the source if 
 * you want.
 */

void debug_print_pos(FILE *stream, struct str_buf substr, char *container, size_t line_num,
		u8 highlight_color, u8 caret_color, LOG_TYPE log_type, const char *msg_fmt, ...);

/* some kind of documentation should go here, probably. 
 */
struct str_buf dbg_escape_str(struct str_buf strbuf);
size_t chresc(char c, char *out);
void freetmp(void);

#endif /* UTIL_H */
