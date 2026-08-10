#include "scanner.h"

char* read_file(const char* filename)
{
    FILE* file;
    char* file_contents; size_t file_size;

    file = fopen(filename , "rb");
    if (file == NULL)
    {
        perror("File does not exist.");
        exit(2);
    }

    fseek(file, 0, SEEK_END);
    long result = ftell(file);
    if (result < 0)
    {
        perror("Failed to get file size");
        exit(1);
    }
    file_size = result;
    fseek(file, 0, SEEK_SET);

    file_contents = malloc(( file_size + 1 ) * sizeof(char));
    if (file_contents == NULL)
    {
        perror("Failed to allocate memory");
        exit(1);
    }
    memset(file_contents, 0, file_size + 1);

    fread(file_contents,  sizeof(char),  file_size,  file);

    fclose(file);
    return file_contents;
}

Scanner init_scanner(const char* filename)
{
    char* txt = read_file(filename);

    Scanner scanner =
    {
        .filename = filename,
        .init = (char*)txt,
        .current = txt,
        .token_list = NULL,
        .diagnostic_component = diagnostic_component_init(),
    }; 

    return scanner;
}

void free_scanner(Scanner* scanner)
{
    arrfree(scanner->token_list);
    free(scanner->init);
    scanner->init = NULL;
    scanner->current = NULL;
    diagnostic_component_free(&scanner->diagnostic_component);
    scanner->diagnostic_component = NULL;
}

bool scanner_is_at_end(Scanner* scanner)
{
    assert(scanner != NULL);

    return scanner->current[0] == '\0';
}

bool scanner_peek_char(Scanner* scanner, char* char_ref)
{
    assert(scanner  != NULL);

    char charr;

    if (scanner_is_at_end(scanner))
    {
        return false;
    }

    charr = scanner->current[0];
    if (char_ref != NULL)
    {
        *char_ref = charr;
    }
    return true;
}

bool scanner_consume_char(Scanner* scanner, char* char_ref)
{
    assert(scanner  != NULL);

    char charr;

    if (!scanner_peek_char(scanner, &charr))
    {
        scanner_throw_err_reached_eof(scanner);
        return false;
    }

    if (is_char_newline(charr))
    {
        scanner->line++;
        scanner->column = 0;
    }
    else
    {
        scanner->column++;
    }
    scanner->current++;

    if (char_ref != NULL)
    {
        *char_ref = charr;
    }
    return true;
}

bool scanner_accept_char(Scanner* scanner, char charr)
{
    assert(scanner  != NULL);

    char recv_char;

    if (!scanner_peek_char(scanner, &recv_char) || recv_char != charr)
    {
        return false;
    }

    assert(scanner_consume_char(scanner, NULL));
    return true;
}

bool scanner_expect_char(Scanner* scanner, char charr)
{
    assert(scanner != NULL);

    if (!scanner_accept_char(scanner, charr))
    {
        scanner_throw_err_unexpected_char(scanner, charr);
        return false;
    }

    return true;
}

bool scanner_peek_str(Scanner* scanner, int length, const char** str_ref)
{
    assert(scanner != NULL);

    const char* str = scanner->current;

    for (int i = 0; i < length; ++i)
    {
        char recv_char;

        if (!scanner_peek_char(scanner, &recv_char))
        {
            return false;
        }
    }

    if (str_ref != NULL)
    {
        *str_ref = str;
    }
    return true;
}

bool scanner_consume_str(Scanner* scanner, int length, const char** str_ref)
{
    assert(scanner != NULL);

    if (!scanner_peek_str(scanner, length, str_ref))
    {
        scanner_throw_err_reached_eof(scanner);
        return false;
    }

    for (int i = 0; i < length; ++i)
    {
        assert(scanner_consume_char(scanner, NULL));
    }
    return true;
}

bool scanner_accept_str(Scanner* scanner, int length, const char* str)
{
    assert(scanner  != NULL);
    assert(str      != NULL);

    const char* recv_str = NULL;

    if (!scanner_peek_str(scanner, length, &recv_str))
    {
        return false;
    }

    // We see whether the received string actually matches what we want
    for (int i = 0; i < length; ++i)
    {
        if (recv_str[i] != str[i])
        {
            return false;
        }
    }

    assert(scanner_consume_str(scanner, length, NULL));
    return true;
}

bool scanner_expect_str(Scanner* scanner, int length, const char* str)
{
    assert(scanner  != NULL);
    assert(str      != NULL);

    if (!scanner_accept_str(scanner, length, str))
    {
        scanner_throw_err_unexpected_str(scanner, length, str);
        return false;
    }

    return true;
}

void scanner_consume_until_inclusive(Scanner* scanner, bool (*predicate)(char))
{
    assert(scanner   != NULL);
    assert(predicate != NULL);

    char charr;

    while (scanner_peek_char(scanner, &charr))
    {
        scanner_consume_char(scanner, NULL);
        if (predicate(charr))
        {
            break;
        }
    }
}

void scanner_consume_until_exclusive(Scanner* scanner, bool (*predicate)(char))
{
    assert(scanner   != NULL);
    assert(predicate != NULL);

    char charr;

    while (scanner_peek_char(scanner, &charr))
    {
        if (predicate(charr))
        {
            break;
        }
        scanner_consume_char(scanner, NULL);
    }
}

void scanner_add_token(Scanner* scanner, Token token)
{
    arrput(scanner->token_list, token);
}

Token scanner_create_init_token(Scanner* scanner, TokenType token_type)
{
    return (Token)
    {
        .type   = token_type      ,
        .start  = scanner->current,
        .line   = scanner->line   ,
        .column = scanner->column ,
        .length = 0               ,
    };
}

ScannerResult scanner_scan_comment(Scanner* scanner, Token* token_ref)
{
    assert(scanner != NULL);

    Token token;
    const char* start = scanner->current;

    token = scanner_create_init_token(scanner, TOKEN_COMMENT);

    if (!scanner_accept_char(scanner, '#'))
    {
        return SCANNER_RESULT_RECOVERABLE_ERROR;
    }

    scanner_consume_until_inclusive(scanner, is_char_newline);
    token.length = scanner->current - start;

    if (token_ref != NULL)
    {
        *token_ref = token;
    }
    return SCANNER_RESULT_SUCCESS;
}

ScannerResult scanner_scan_str(Scanner* scanner, Token* token_ref)
{
    assert(scanner != NULL);

    Token token;
    char charr;
    const char* start = scanner->current;
    bool is_escaping = false;

    token = scanner_create_init_token(scanner, TOKEN_STRING);

    if (!scanner_accept_char(scanner, '"'))
    {
        return SCANNER_RESULT_RECOVERABLE_ERROR;
    }

    do
    {
        if (!scanner_peek_char(scanner, &charr))
        {
            scanner_throw_err_str_non_terminating(scanner);
            return SCANNER_RESULT_IRRECOVERABLE_ERROR;
        }
        assert(scanner_consume_char(scanner, NULL));

        if (charr == '\\')
        {
            is_escaping = is_escaping ? false : true;
        }
    }
    while (charr != '"' || is_escaping);

    token.length = scanner->current - start;

    if (token_ref != NULL)
    {
        *token_ref = token;
    }
    return SCANNER_RESULT_SUCCESS;
}

ScannerResult scanner_scan_number(Scanner* scanner, Token* token_ref)
{
    assert(scanner != NULL);

    Token token;
    char charr;
    const char* start = scanner->current;

    token = scanner_create_init_token(scanner, TOKEN_INTEGER);
    if (!scanner_peek_char(scanner, &charr) || not_digit(charr))
    {
        return SCANNER_RESULT_RECOVERABLE_ERROR;
    }
    scanner_consume_until_exclusive(scanner, not_digit);
    scanner_peek_char(scanner, &charr);

    if (charr == '.')
    {
        assert(scanner_consume_char(scanner, NULL));
        token.type = TOKEN_NUMBER;

        if (!scanner_consume_char(scanner, &charr))
        {
            return SCANNER_RESULT_IRRECOVERABLE_ERROR;
        }

        if (not_digit(charr))
        {
            scanner_throw_err_expected_digit(scanner);
            return SCANNER_RESULT_IRRECOVERABLE_ERROR;
        }

        scanner_consume_until_exclusive(scanner, not_digit);
    }

    token.length = scanner->current - start;

    if (token_ref != NULL)
    {
        *token_ref = token;
    }
    return SCANNER_RESULT_SUCCESS;
}

ScannerResult scanner_scan_identifier(Scanner* scanner, Token* token_ref)
{
    assert(scanner != NULL);

    Token token;
    char charr;
    const char* start = scanner->current;

    token = scanner_create_init_token(scanner, TOKEN_IDENTIFIER);

    if (!scanner_peek_char(scanner, &charr) || !is_identifier_start(charr))
    {
        return SCANNER_RESULT_RECOVERABLE_ERROR;
    }
    assert(scanner_consume_char(scanner, NULL));

    scanner_consume_until_exclusive(scanner, not_identifier_middle);

    if (scanner_peek_char(scanner, &charr) && is_identifier_end(charr))
    {
        assert(scanner_consume_char(scanner, NULL));
    }

    token.length = scanner->current - start;

    if (token_ref != NULL)
    {
        *token_ref = token;
    }
    return SCANNER_RESULT_SUCCESS;
}

ScannerResult scanner_scan_keyword(Scanner* scanner, const char* keyword, TokenType token_type, Token* token_ref)
{
    assert(scanner != NULL);
    assert(keyword != NULL);

    Token token;
    const char* start = scanner->current;

    token = scanner_create_init_token(scanner, token_type);

    // bool scanner_accept_str(Scanner* scanner, int length, char* str)
    if (!scanner_accept_str(scanner, strlen(keyword), keyword))
    {
        return SCANNER_RESULT_RECOVERABLE_ERROR;
    }

    token.length = scanner->current - start;
    if (token_ref != NULL)
    {
        *token_ref = token;
    }
    return SCANNER_RESULT_SUCCESS;
}

ScannerResult scanner_scan_keywords(Scanner* scanner, Token* token_ref)
{
#define SCAN_KEYWORD(str, token_type) \
    HANDLE_SCANNER_RESULT_BASE_CASE \
        (scanner_scan_keyword(scanner, (str), (token_type) , token_ref))

    assert(scanner != NULL);

    // One character tokens
    SCAN_KEYWORD("(" , TOKEN_LEFT_PAREN  );
    SCAN_KEYWORD(")" , TOKEN_RIGHT_PAREN );
    SCAN_KEYWORD("{" , TOKEN_LEFT_BRACE  );
    SCAN_KEYWORD("}" , TOKEN_RIGHT_BRACE );
    SCAN_KEYWORD("[" , TOKEN_LEFT_SQUARE );
    SCAN_KEYWORD("]" , TOKEN_RIGHT_SQUARE);
    SCAN_KEYWORD("|" , TOKEN_PIPE        );
    SCAN_KEYWORD("," , TOKEN_COMMA       );
    SCAN_KEYWORD(";" , TOKEN_SEMICOLON   );
    SCAN_KEYWORD("_" , TOKEN_UNDERSCORE  );
    SCAN_KEYWORD("\n", TOKEN_NEWLINE     );

    // One or more character tokens
    SCAN_KEYWORD("!=", TOKEN_BANG_EQUAL   );
    SCAN_KEYWORD("!" , TOKEN_BANG         );
    SCAN_KEYWORD("==", TOKEN_EQUAL_EQUAL  );
    SCAN_KEYWORD("=>", TOKEN_EQUAL_GREATER);
    SCAN_KEYWORD("=" , TOKEN_EQUAL        );
    SCAN_KEYWORD("+=", TOKEN_PLUS_EQUAL   );
    SCAN_KEYWORD("++", TOKEN_PLUS_PLUS    );
    SCAN_KEYWORD("+" , TOKEN_PLUS         );
    SCAN_KEYWORD("-=", TOKEN_MINUS_EQUAL  );
    SCAN_KEYWORD("--", TOKEN_MINUS_MINUS  );
    SCAN_KEYWORD("->", TOKEN_MINUS_GREATER);
    SCAN_KEYWORD("-" , TOKEN_MINUS        );
    SCAN_KEYWORD("*=", TOKEN_STAR_EQUAL   );
    SCAN_KEYWORD("*" , TOKEN_STAR         );
    SCAN_KEYWORD("/=", TOKEN_SLASH_EQUAL  );
    SCAN_KEYWORD("/" , TOKEN_SLASH        );
    SCAN_KEYWORD("%=", TOKEN_PERCENT_EQUAL);
    SCAN_KEYWORD("%" , TOKEN_PERCENT      );
    SCAN_KEYWORD("<=", TOKEN_LESS_EQUAL   );
    SCAN_KEYWORD("<" , TOKEN_LESS         );
    SCAN_KEYWORD(">=", TOKEN_GREATER_EQUAL);
    SCAN_KEYWORD(">" , TOKEN_GREATER      );
    SCAN_KEYWORD("..", TOKEN_DOT_DOT      );
    SCAN_KEYWORD("." , TOKEN_DOT          );
    SCAN_KEYWORD("::", TOKEN_COLON_COLON  );
    SCAN_KEYWORD(":" , TOKEN_COLON        );

    // Text-based keywords
    SCAN_KEYWORD("Nil"     , TOKEN_NIL_T   );
    SCAN_KEYWORD("Nat"     , TOKEN_NAT     );
    SCAN_KEYWORD("Bool"    , TOKEN_BOOL    );
    SCAN_KEYWORD("Int"     , TOKEN_INT     );
    SCAN_KEYWORD("Real"    , TOKEN_REAL    );
    SCAN_KEYWORD("let"     , TOKEN_LET     );
    SCAN_KEYWORD("loop"    , TOKEN_LOOP    );
    SCAN_KEYWORD("type"    , TOKEN_TYPE    );
    SCAN_KEYWORD("true"    , TOKEN_TRUE    );
    SCAN_KEYWORD("end"     , TOKEN_END     );
    SCAN_KEYWORD("elif"    , TOKEN_ELIF    );
    SCAN_KEYWORD("else"    , TOKEN_ELSE    );
    SCAN_KEYWORD("nil"     , TOKEN_NIL_V   );
    SCAN_KEYWORD("not"     , TOKEN_NOT     );
    SCAN_KEYWORD("false"   , TOKEN_FALSE   );
    SCAN_KEYWORD("for"     , TOKEN_FOR     );
    SCAN_KEYWORD("fn"      , TOKEN_FN      );
    SCAN_KEYWORD("and"     , TOKEN_AND     );
    SCAN_KEYWORD("alias"   , TOKEN_ALIAS   );
    SCAN_KEYWORD("or"      , TOKEN_OR      );
    SCAN_KEYWORD("do"      , TOKEN_DO      );
    SCAN_KEYWORD("if"      , TOKEN_IF      );
    SCAN_KEYWORD("in"      , TOKEN_IN      );
    SCAN_KEYWORD("while"   , TOKEN_WHILE   );
    SCAN_KEYWORD("break"   , TOKEN_BREAK   );
    SCAN_KEYWORD("continue", TOKEN_CONTINUE);
    SCAN_KEYWORD("return"  , TOKEN_RETURN  );
    SCAN_KEYWORD("match"   , TOKEN_MATCH   );

    return SCANNER_RESULT_RECOVERABLE_ERROR;

#undef SCAN_KEYWORD
}

bool scanner_convert_identifier_to_keyword(Scanner* scanner, const char* keyword, TokenType token_type, Token* token_ref)
{
    assert(scanner   != NULL);
    assert(keyword   != NULL);
    assert(token_ref != NULL);
    assert(token_ref->type == TOKEN_IDENTIFIER);

    int length_keyword = strlen(keyword);
    int length_token   = token_ref->length;

    if (length_keyword != length_token)
    {
        return false;
    }

    for (int i = 0; i < length_keyword; ++i)
    {
        char charr_keyword = keyword[i];
        char charr_token   = token_ref->start[i];

        if (charr_keyword != charr_token)
        {
            return false;
        }
    }

    token_ref->type = token_type;
    return true;
}

// The size of the function is excessive, since text keywords, such as 'let', 'while',
// are capable of being identifiers.
// Whereas keywords such as '+=' are not, there for they cannot be converted.
// I'm keeping all of these keywords for simplicity.
bool scanner_convert_identifier_to_keywords(Scanner* scanner, Token* token_ref)
{
#define CONVERT(str, token_type) \
    if (scanner_convert_identifier_to_keyword(scanner, (str), (token_type), token_ref)) return true

    assert(scanner   != NULL);
    assert(token_ref != NULL);
    assert(token_ref->type == TOKEN_IDENTIFIER);

    // One character tokens
    CONVERT("(" , TOKEN_LEFT_PAREN  );
    CONVERT(")" , TOKEN_RIGHT_PAREN );
    CONVERT("{" , TOKEN_LEFT_BRACE  );
    CONVERT("}" , TOKEN_RIGHT_BRACE );
    CONVERT("[" , TOKEN_LEFT_SQUARE );
    CONVERT("]" , TOKEN_RIGHT_SQUARE);
    CONVERT("|" , TOKEN_PIPE        );
    CONVERT("," , TOKEN_COMMA       );
    CONVERT(";" , TOKEN_SEMICOLON   );
    CONVERT("_" , TOKEN_UNDERSCORE  );
    CONVERT("\n", TOKEN_NEWLINE     );

    // One or more character tokens
    CONVERT("!=", TOKEN_BANG_EQUAL   );
    CONVERT("!" , TOKEN_BANG         );
    CONVERT("==", TOKEN_EQUAL_EQUAL  );
    CONVERT("=>", TOKEN_EQUAL_GREATER);
    CONVERT("=" , TOKEN_EQUAL        );
    CONVERT("+=", TOKEN_PLUS_EQUAL   );
    CONVERT("++", TOKEN_PLUS_PLUS    );
    CONVERT("+" , TOKEN_PLUS         );
    CONVERT("-=", TOKEN_MINUS_EQUAL  );
    CONVERT("--", TOKEN_MINUS_MINUS  );
    CONVERT("->", TOKEN_MINUS_GREATER);
    CONVERT("-" , TOKEN_MINUS        );
    CONVERT("*=", TOKEN_STAR_EQUAL   );
    CONVERT("*" , TOKEN_STAR         );
    CONVERT("/=", TOKEN_SLASH_EQUAL  );
    CONVERT("/" , TOKEN_SLASH        );
    CONVERT("%=", TOKEN_PERCENT_EQUAL);
    CONVERT("%" , TOKEN_PERCENT      );
    CONVERT("<=", TOKEN_LESS_EQUAL   );
    CONVERT("<" , TOKEN_LESS         );
    CONVERT(">=", TOKEN_GREATER_EQUAL);
    CONVERT(">" , TOKEN_GREATER      );
    CONVERT("..", TOKEN_DOT_DOT      );
    CONVERT("." , TOKEN_DOT          );
    CONVERT("::", TOKEN_COLON_COLON  );
    CONVERT(":" , TOKEN_COLON        );

    // Text-based keywords
    CONVERT("Nil"     , TOKEN_NIL_T   );
    CONVERT("Nat"     , TOKEN_NAT     );
    CONVERT("Bool"    , TOKEN_BOOL    );
    CONVERT("Int"     , TOKEN_INT     );
    CONVERT("Real"    , TOKEN_REAL    );
    CONVERT("let"     , TOKEN_LET     );
    CONVERT("loop"    , TOKEN_LOOP    );
    CONVERT("type"    , TOKEN_TYPE    );
    CONVERT("true"    , TOKEN_TRUE    );
    CONVERT("end"     , TOKEN_END     );
    CONVERT("elif"    , TOKEN_ELIF    );
    CONVERT("else"    , TOKEN_ELSE    );
    CONVERT("nil"     , TOKEN_NIL_V   );
    CONVERT("not"     , TOKEN_NOT     );
    CONVERT("false"   , TOKEN_FALSE   );
    CONVERT("for"     , TOKEN_FOR     );
    CONVERT("fn"      , TOKEN_FN      );
    CONVERT("and"     , TOKEN_AND     );
    CONVERT("alias"   , TOKEN_ALIAS   );
    CONVERT("or"      , TOKEN_OR      );
    CONVERT("do"      , TOKEN_DO      );
    CONVERT("if"      , TOKEN_IF      );
    CONVERT("in"      , TOKEN_IN      );
    CONVERT("while"   , TOKEN_WHILE   );
    CONVERT("break"   , TOKEN_BREAK   );
    CONVERT("continue", TOKEN_CONTINUE);
    CONVERT("return"  , TOKEN_RETURN  );
    CONVERT("match"   , TOKEN_MATCH   );

    return true;

#undef CONVERT
}

ScannerResult scanner_scan_token(Scanner* scanner, Token* token_ref)
{
    assert(scanner != NULL);

    scanner_consume_until_exclusive(scanner, not_whitespace);

    HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_comment    (scanner, token_ref));
    HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_str        (scanner, token_ref));
    HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_number     (scanner, token_ref));
    // HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_identifier (scanner, token_ref));
    // HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_keywords   (scanner, token_ref));

    switch (scanner_scan_identifier(scanner, token_ref))
    {
        case SCANNER_RESULT_SUCCESS            :
        {
            scanner_convert_identifier_to_keywords(scanner, token_ref);
            return SCANNER_RESULT_SUCCESS;
        }

        case SCANNER_RESULT_RECOVERABLE_ERROR  :
        {
            HANDLE_SCANNER_RESULT_BASE_CASE(scanner_scan_keywords   (scanner, token_ref));
            return SCANNER_RESULT_RECOVERABLE_ERROR;
        }

        case SCANNER_RESULT_IRRECOVERABLE_ERROR:
        {
            return SCANNER_RESULT_RECOVERABLE_ERROR;
        }
    }

    UNREACHABLE;
}

bool scanner_scan_tokens(Scanner* scanner)
{
    assert(scanner != NULL);

    Token token;
    bool tokens_left = true;
    bool error_found = false;

    do
    {
        switch (scanner_scan_token(scanner, &token))
        {
            case SCANNER_RESULT_SUCCESS            : scanner_add_token(scanner, token); break;
            case SCANNER_RESULT_RECOVERABLE_ERROR  :
            {
                char charr;

                if (!scanner_peek_char(scanner, &charr))
                {
                    tokens_left = false;
                    break;
                }
                scanner_throw_err_unexpected_char(scanner, charr);

                error_found = true;
                scanner_consume_until_inclusive(scanner, is_char_newline);
                break;
            }
            case SCANNER_RESULT_IRRECOVERABLE_ERROR:
            {
                error_found = true;
                scanner_consume_until_inclusive(scanner, is_char_newline);
                break;
            }
        }
    }
    while (tokens_left);

    // For some reason, our parser doesn't use TOKEN_EOF, and appending it actually breaks things.
    // token = scanner_create_init_token(scanner, TOKEN_EOF);
    // scanner_add_token(scanner, token);

    return !error_found;
}

bool is_char_newline(char charr)
{
    return charr == '\n';
}

bool is_digit(char charr)
{
    switch (charr)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return true;
        default: return false;
    }
}

bool not_digit(char charr)
{
    return !is_digit(charr);
}

bool is_alpha(char charr)
{
    return (charr >= 'a' && charr <= 'z') ||
           (charr >= 'A' && charr <= 'Z');
}

bool is_whitespace(char charr)
{
    switch (charr)
    {
        case ' ':
        case '\t':
        case '\r':
            return true;
        default: return false;
    }
}

bool not_whitespace(char charr)
{
    return !is_whitespace(charr);
}

bool is_identifier_start(char charr)
{
    return is_alpha(charr)
        || (charr == '_');
}

bool is_identifier_middle(char charr)
{
    return is_identifier_start(charr)
        || is_digit(charr);
}

bool not_identifier_middle(char charr)
{
    return !is_identifier_middle(charr);
}

bool is_identifier_end(char charr)
{
    return is_identifier_middle(charr);
}

Span scanner_get_scanner_span(Scanner* scanner)
{
    return (Span)
    {
        .line   = scanner->line  ,
        .column = scanner->column,
        .length = 1              ,
    };
}

void scanner_throw_err_reached_eof(Scanner* scanner)
{
    assert(scanner != NULL);

    diagnostic_component_push_err(scanner->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_REACHED_EOF,
        .span = scanner_get_scanner_span(scanner),
    });
}

void scanner_throw_err_unexpected_char(Scanner* scanner, char charr)
{
    assert(scanner != NULL);

    diagnostic_component_push_err(scanner->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_UNEXPECTED_CHAR,
        .span = scanner_get_scanner_span(scanner),
        .err.unexpected_char =
        {
            .charr = charr,
        },
    });
}

void scanner_throw_err_unexpected_str(Scanner* scanner, int length, const char* str)
{
    assert(scanner != NULL);

    diagnostic_component_push_err(scanner->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_UNEXPECTED_STR,
        .span = scanner_get_scanner_span(scanner),
        .err.unexpected_str =
        {
            .length = length,
            .str = (const char*) arena_push
                (&scanner->diagnostic_component->arena, (void*) str, sizeof(char) * length),
        }
    });
}

// TODO: Make it so that it shows the string in question
void scanner_throw_err_str_non_terminating(Scanner* scanner)
{
    assert(scanner != NULL);

    diagnostic_component_push_err(scanner->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_STR_NON_TERMINATING,
        .span = scanner_get_scanner_span(scanner),
    });
}

void scanner_throw_err_expected_digit(Scanner* scanner)
{
    assert(scanner != NULL);

    diagnostic_component_push_err(scanner->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPECTED_DIGIT,
        .span = scanner_get_scanner_span(scanner),
    });
}
