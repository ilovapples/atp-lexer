#include "lexer.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "types.h"
#include "util.h"
#include "args.h"
#include "c-hashmap/map.h"

char *SRC_PATH_L = NULL;

void print_usage_msg_lexer(void)
{
	error(1, "usage: %s [options] <in_file>\n\n"

		   "  -d, --debug      enable debug output\n"
			, PROG_NAME);
}

static struct str_buf source_code = {0};
static bool stream_will_terminate = false,
		lexer_is_initialized = false;
static char *token_start_pos = NULL;
static bool is_escaped_char = false;
static u8 n_consec_backslashes = 0;
static size_t n_dquotes = 0,
		  n_squotes = 0;
static char *str_start = NULL,
		*chr_start = NULL;
static size_t str_start_line = 0,
		  chr_start_line = 0;
static size_t line_n = 1;
static size_t token_n = 0;
#define IN_STRING() (n_dquotes % 2 == 1)
#define IN_CHAR() (n_squotes %2 == 1)
static size_t in_char_for = 0;
#define IS_ESCAPED() (n_consec_backslashes % 2 == 1)

hashmap *keyword_map;

void keyword_map_init(void)
{
	keyword_map = hashmap_create();

	hashmap_set(keyword_map, hashmap_str_lit("func"), FUNC_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("return"), RETURN_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("obtain"), RESERVED_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("s8"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("s16"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("s32"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("s64"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("u8"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("u16"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("u32"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("u64"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("char"), TYPE_KEYWORD);
	hashmap_set(keyword_map, hashmap_str_lit("string"), TYPE_KEYWORD); /* <- a char array (we're not using C-strings) */
	hashmap_set(keyword_map, hashmap_str_lit("slice"), TYPE_KEYWORD);
	/* ^ a structure containing a length and a pointer to a value in an array ^ */
}

void lexer_init(struct str_buf contents_in)
{
	flogf(LOG_DEBUG, stdout, "initializing lexer...\n");
	source_code = contents_in;
	token_start_pos = source_code.buf;

	flogf(LOG_DEBUG, stdout, "initializing keyword hashmap...\n");
	keyword_map_init();

	lexer_is_initialized = true;
	flogf(LOG_DEBUG, stdout, "successfully initialized lexer.\n");
}

u32 errflags = 0;

bool is_null_token(Token token)
{
	return (token.type == FileEndToken);
}

TokenSubType keyword_type(struct str_buf ident)
{
	uintptr_t ret = 0;
	if (hashmap_get(keyword_map, ident.buf, ident.len, &ret) == 0)
		return NORMAL_IDENTIFIER;
	return (TokenSubType) ret;
}

bool skipped_int_literal_prefix = false;

#include "is_digit.c"

size_t int_literal_valid_length(char *lit_start, TokenSubType *subtype_out)
{
	bool (*is_digit) (char c);
	char literal_type_string[11+1];
	if (strncmp(lit_start, "0d", 2) == 0)
	{
		*subtype_out = DEC_INT_LITERAL;
		is_digit = is_dec_digit;
		skipped_int_literal_prefix = true;
		strncpy(literal_type_string, "decimal", 8);
	} else if (strncmp(lit_start, "0x", 2) == 0)
	{
		*subtype_out = HEX_INT_LITERAL;
		is_digit = is_hex_digit;
		skipped_int_literal_prefix = true;
		strncpy(literal_type_string, "hexadecimal", 12);
	} else if (strncmp(lit_start, "0o", 2) == 0)
	{
		*subtype_out = OCT_INT_LITERAL;
		is_digit = is_oct_digit;
		skipped_int_literal_prefix = true;
		strncpy(literal_type_string, "octal", 6);
	} else if (strncmp(lit_start, "0b", 2) == 0)
	{
		*subtype_out = BIN_INT_LITERAL;
		is_digit = is_bin_digit;
		skipped_int_literal_prefix = true;
		strncpy(literal_type_string, "binary", 7);
	} else if (*lit_start == '0' && isalpha(*(lit_start+1)))
	{
		debug_print_pos(stderr, strbuflit(lit_start, 2, SRC_PATH_L), source_code.buf, line_n,
				ERR_COLOR, ERR_COLOR,
				LOG_ERR, "invalid integer literal type:\n");
		*subtype_out = ERROR_TOKEN;
		seterr(INVALID_INT_LITERAL);
		return 0;
	} else if (*lit_start != '0' && is_dec_digit(*lit_start))
	{
		*subtype_out = DEC_INT_LITERAL;
		is_digit = is_dec_digit;
		skipped_int_literal_prefix = false;
		strncpy(literal_type_string, "decimal", 8);
	} else 
		return 0;

	char *start_pos = lit_start + ((skipped_int_literal_prefix) ? 2 : 0);
	char *pos = start_pos;
	while ((*is_digit)(*pos)) 
		pos++;
	size_t ret_len = pos - start_pos;

	if (ret_len > 0 && isalnum(*pos))
	{
		debug_print_pos(stderr, strbuflit(pos, 1, SRC_PATH_L), source_code.buf, line_n,
				ERR_COLOR, ERR_COLOR,
				LOG_ERR, "trailing character following %s integer literal:\n",
				literal_type_string);
		seterr(INT_LITERAL_HAS_TRAILING_CHAR);
	} else if (ret_len == 0 && isxdigit(*pos+1) && *subtype_out != ERROR_TOKEN)
	{
		debug_print_pos(stderr, strbuflit(pos, 1, SRC_PATH_L), source_code.buf, line_n,
				ERR_COLOR, ERR_COLOR,
				LOG_ERR, "%s radix specifier immediately followed by non-%s digit:\n",
				literal_type_string, literal_type_string);
		seterr(INT_LITERAL_HAS_NO_VALID_DIGITS);
	}

	return ret_len;
}

Token next_token(void)
{
	if (stream_will_terminate)
		return NULL_TOKEN;

	if (!lexer_is_initialized) {
		flogf(LOG_ERR, stderr, "`lexer_init` must be called prior to accessing the token stream.\n");
		seterr(UNINITIALIZED_LEXER);
		return NULL_TOKEN;
	}

	token_n++;

	flogf(LOG_DEBUG, stdout, "accessing token starting from char %zu\n", token_start_pos - source_code.buf);

	Token ret = {0};
	ret.value.buf = token_start_pos;
	ret.value.len = 1;
	ret.value.capacity = (source_code.buf + source_code.len) - token_start_pos;
	flogf(LOG_DEBUG, stdout, "Set initial values for token #%d.\n", token_n);

	// skip any whitespace at the start
	while (!IN_STRING() && !IN_CHAR() && isspace(*ret.value.buf))
	{
		if (*ret.value.buf == '\n')
			line_n++;
		token_start_pos++;
		ret.value.buf++;
	}
	if (*ret.value.buf == '\0')
		return NULL_TOKEN;
	flogf(LOG_DEBUG, stdout, "Skipped initial whitespace for token #%d.\n", token_n);
	struct str_buf escaped_5_chars = dbg_escape_str(strbuflit(ret.value.buf, MIN(5, ret.value.capacity), SRC_PATH_L));
	flogf(LOG_DEBUG, stdout, "Next 5 (valid) chars of token #%d: '%.*s'\n", token_n,
			escaped_5_chars.len, escaped_5_chars.buf);
	freetmp();

	char first_char = *ret.value.buf;
	if (first_char == '"' || first_char == '\'') {
		goto check_char_string;
	}

	flogf(LOG_DEBUG, stdout, "Checking if token #%d constitutes the beginning of an escape sequence.\n", token_n);

	// check if first_char constitutes the start of an escape sequence
	if (first_char == '\\' && (IN_STRING() || IN_CHAR()))
	{
		if ((++n_consec_backslashes) % 2 == 1)
		{
			is_escaped_char = true;
			ret.value.len = 1;
			ret.type = EscapeCodeStartToken;
			ret.subtype = NOT_IDENTIFIER;
			debug_print_pos(stdout, strbuflit(ret.value.buf, 1, SRC_PATH_L),
					source_code.buf, line_n, 
					DEBUG_COLOR, DEBUG_COLOR,
					LOG_DEBUG, "non-escaped backslash in string/char:\n");

			goto func_end;
		}
	}

	flogf(LOG_DEBUG, stdout, "Checking if token #%d is in a string.\n", token_n);
	// check if the character is within a string literal
	if (IN_STRING()) {
		flogf(LOG_DEBUG, stdout, "Token #%d is in a string.\n", token_n);
		ret.value.len = 1;
		ret.type = WithinStringToken;
		ret.subtype = NOT_IDENTIFIER;

		goto func_end;
	}

	flogf(LOG_DEBUG, stdout, "Checking if token #%d is in a character.\n", token_n);
	// check if the character is within a character literal
	if (IN_CHAR()) {
		flogf(LOG_DEBUG, stdout, "Token #%d is in a character.\n", token_n);
		ret.value.len = 1;
		ret.type = WithinCharToken;
		ret.subtype = NOT_IDENTIFIER;
		if (++in_char_for > 1)
		{
			flogf(LOG_DEBUG, stdout, "Token #%d makes the character literal too long.\n", token_n);
			debug_print_pos(stderr, strbuflit(chr_start, 1, SRC_PATH_L),
					source_code.buf, chr_start_line,
					ERR_COLOR, ERR_COLOR,
					LOG_ERR, "unterminated character literal:\n");
			seterr(EXCESSIVE_CHAR_LITERAL);
		}

		goto func_end;
	}

	// check if token is an integer literal
	TokenSubType int_lit_type;
	size_t int_lit_len = int_literal_valid_length(ret.value.buf, &int_lit_type);
	if (int_lit_len > 0)
	{
		if (!geterr(INVALID_INT_LITERAL))
			flogf(LOG_DEBUG, stdout, "token #%d is a valid integer literal.\n", token_n);
		ret.type = IntegerLiteralToken;
		ret.value.len = int_lit_len;
		if (skipped_int_literal_prefix) {
			flogf(LOG_DEBUG, stdout, "token #%d has a 2-character prefix that has been skipped.\n", token_n);
			token_start_pos += 2;
			ret.value.buf += 2;
		}
		ret.subtype = int_lit_type;
		goto func_end;
	}

	// check if token is an identifier
	flogf(LOG_DEBUG, stdout, "Checking if token #%d contains a valid identifier...\n", token_n);

	// get identifier length
	bool can_be_identifier = can_be_identifier = (isalpha(*ret.value.buf) || *ret.value.buf == '_');
	char *pos = ret.value.buf+1;
	while (isalnum(*pos) || *pos == '_')
		pos++;

	if (can_be_identifier && pos > ret.value.buf)
	{
		flogf(LOG_DEBUG, stdout, "token #%d is a valid identifier.\n", token_n);
		ret.type = IdentifierToken;
		ret.value.len = pos - ret.value.buf;
		ret.subtype = keyword_type(ret.value);
		goto func_end;
	}
	
	if (strncmp(ret.value.buf, "...", 3) == 0)
	{
		ret.type = VariableArgumentIndicatorToken;
		ret.subtype = NOT_IDENTIFIER;
		ret.value.len = 3;
		goto func_end;
	}

	if (strncmp(ret.value.buf, "->", 2) == 0)
	{
		ret.type = ReturnTypeIndicatorToken;
		ret.subtype = NOT_IDENTIFIER;
		ret.value.len = 2;
		goto func_end;
	}
	if (strncmp(ret.value.buf, ":=", 2) == 0)
	{
		ret.type = VarTypeInferInitToken;
		ret.subtype = NOT_IDENTIFIER;
		ret.value.len = 2;
		goto func_end;
	}
	if ((strncmp(ret.value.buf, "==", 2) == 0)
	 || (strncmp(ret.value.buf, "!=", 2) == 0)
	 || (strncmp(ret.value.buf, "<=", 2) == 0)
	 || (strncmp(ret.value.buf, ">=", 2) == 0)
	 || (strncmp(ret.value.buf, "+=", 2) == 0)
	 || (strncmp(ret.value.buf, "-=", 2) == 0)
	 || (strncmp(ret.value.buf, "*=", 2) == 0)
	 || (strncmp(ret.value.buf, "/=", 2) == 0)
	 || (strncmp(ret.value.buf, "%=", 2) == 0)
	 || (strncmp(ret.value.buf, "&=", 2) == 0)
	 || (strncmp(ret.value.buf, "|=", 2) == 0)
	 || (strncmp(ret.value.buf, "^=", 2) == 0)
	 || (strncmp(ret.value.buf, "~=", 2) == 0)
	 || (strncmp(ret.value.buf, "<<", 2) == 0)
	 || (strncmp(ret.value.buf, ">>", 2) == 0)
	 ) {
		ret.value.len = 2;
		ret.type = OperatorToken;
		ret.subtype = NOT_IDENTIFIER;
		goto func_end;
	}

	switch (*ret.value.buf) {
	case '{':
		ret.type = StartBlockToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case '}':
		ret.type = EndBlockToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case '[':
		ret.type = StartBracketToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case ']':
		ret.type = EndBracketToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case '(':
		ret.type = StartParenToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case ')':
		ret.type = EndParenToken;
		ret.subtype = GROUPING_TOKEN;
		goto func_end;
	case ',':
		ret.type = ItemSeparatorToken;
		ret.subtype = NOT_IDENTIFIER;
		goto func_end;
	case ';':
		ret.type = EndStatementToken;
		ret.subtype = NOT_IDENTIFIER;
		goto func_end;
	case '=':
	case '!':
	case '<':
	case '>':
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '@':
	case '&':
	case '|':
	case '^':
	case '~':
	case ':':
	case '?':
	case '.':
		ret.type = OperatorToken;
		ret.subtype = NOT_IDENTIFIER;
		goto func_end;
	}

check_char_string:
	// do this at the end so that the following call will be able to use these values
	if (first_char == '"' && !IS_ESCAPED() && !IN_CHAR())
	{
		if (++n_dquotes % 2 == 0)
		{
			// string end
			str_start = NULL;
			str_start_line = 0;
			ret.type = EndStringToken;
		} else
		{
			// string start
			str_start = ret.value.buf;
			str_start_line = line_n;
			ret.type = StartStringToken;
		}
		goto func_end;
	}
	if (first_char == '\'' && !IS_ESCAPED() && !IN_STRING())
	{
		if (++n_squotes % 2 == 0)
		{
			// char end
			chr_start = NULL;
			chr_start_line = 0;
			ret.type = EndCharToken;
		} else
		{
			// char start
			str_start = ret.value.buf;
			str_start_line = line_n;
			ret.type = StartCharToken;
		}
		goto func_end;
	}
	flogf(LOG_DEBUG, stdout, "Properly incremented quote counters for token #%d.\n", token_n);



	if (!(first_char == '\\' && (IN_STRING() || IN_CHAR())))
		n_consec_backslashes = 0;

func_end:
	token_start_pos += ret.value.len;
	stream_will_terminate = (token_start_pos > (source_code.buf + source_code.len));

	return ret;
}
