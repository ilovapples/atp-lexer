#ifndef LEXER_H
#define LEXER_H

#include "util.h"
#include "types.h"

/* maybe token list is stored as a doubly-linked list? 
 * supposedly implementing it as a stream is quite helpful.
 */
typedef enum {
	MiscToken = 0x00,
      IdentifierToken = 0x04, /* e.g. main, swap, a (also matches keywords, which specified by subtype > 0 */
      StartStringToken = 0x08, /* '"' */
      WithinStringToken = 0x09, /* any non-EscapeCode character between a StartString and EndString token */
	EscapeCodeStartToken, /* a backslash that is the nth consecutive backslash iff n is odd */
      EscapeCodeToken, /* any character within a string that is part of an escape sequence */
      EndStringToken = 0x10, /* '"' */
      StartCharToken = 0x14, /* '\''*/
      WithinCharToken = 0x15, /* any character between StartChar and EndChar tokens. */
      EndCharToken = 0x18, /* '\'' */
      VarTypeInferInitToken = 0x1C, /* :=  (RESERVED) */
      OperatorToken = 0x20, /* e.g. *, +, @ */
      StartBlockToken = 0x24, /* { */
      EndBlockToken = 0x26, /* } */
      StartParenToken = 0x2A, /* ( */
      EndParenToken = 0x2C, /* ) */
      StartBracketToken = 0x30, /* [ */
      EndBracketToken = 0x32, /* ] */
      EndStatementToken = 0x3A, /* ; */
	ItemSeparatorToken = 0x40, /* , */
	VariableArgumentIndicatorToken = 0x41, /* ... */
      ReturnTypeIndicatorToken = 0x44, /* -> */
      IntegerLiteralToken = 0x48, /* e.g. 531, 91 */
	FileEndToken = 0x4C,
} TokenType;

typedef enum {
	ERROR_TOKEN = 0x00,
	GROUPING_TOKEN,
	NOT_IDENTIFIER, /* anything that isn't on the list below */
	NORMAL_IDENTIFIER,
	/* keyword types */
	FUNC_KEYWORD = 0x10,
	TYPE_KEYWORD,
	RETURN_KEYWORD,
	RESERVED_KEYWORD, /* keywords that aren't be used just yet, but might be eventually */
	/* integer literal types */
	DEC_INT_LITERAL = 0x50,
	HEX_INT_LITERAL,
	OCT_INT_LITERAL,
	BIN_INT_LITERAL,
} TokenSubType;

#define seterr(f) (errflags |= (1<<(f)))
#define geterr(f) ((errflags & (1<<(f))) != 0)
enum {
	UNINITIALIZED_LEXER,
	INVALID_INT_LITERAL,
	INT_LITERAL_HAS_TRAILING_CHAR,
	INT_LITERAL_HAS_NO_VALID_DIGITS,
	EXCESSIVE_CHAR_LITERAL,
};

typedef struct {
      TokenType type;
	TokenSubType subtype;
      struct str_buf value; /* preferably a pointer to a spot in the buffer that holds the value of the token */
} Token;

#define NULL_TOKEN ((Token) { FileEndToken, NOT_IDENTIFIER, strbuflit(NULL, 0, NULL) })

void print_usage_msg_lexer(void);
void parse_args_lexer(s32 argc, char **argv);

void lexer_init(struct str_buf contents_in);
bool is_null_token(Token token);
Token next_token(void);

#endif /* LEXER_H */
