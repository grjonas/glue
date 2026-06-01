#include "scanner.h"

const char* token_type_name(TokenType type)
{
    switch (type)
    {
        case TOKEN_LEFT_PAREN:    return "TOKEN_LEFT_PAREN";
        case TOKEN_RIGHT_PAREN:   return "TOKEN_RIGHT_PAREN";
        case TOKEN_LEFT_BRACE:    return "TOKEN_LEFT_BRACE";
        case TOKEN_RIGHT_BRACE:   return "TOKEN_RIGHT_BRACE";
        case TOKEN_COMMA:         return "TOKEN_COMMA";
        case TOKEN_MINUS:         return "TOKEN_MINUS";
        case TOKEN_PLUS:          return "TOKEN_PLUS";
        case TOKEN_SEMICOLON:     return "TOKEN_SEMICOLON";
        case TOKEN_STAR:          return "TOKEN_STAR";
        case TOKEN_NEWLINE:       return "TOKEN_NEWLINE";

        case TOKEN_EQUAL:         return "TOKEN_EQUAL";
        case TOKEN_EQUAL_EQUAL:   return "TOKEN_EQUAL_EQUAL";
        case TOKEN_GREATER:       return "TOKEN_GREATER";
        case TOKEN_GREATER_EQUAL: return "TOKEN_GREATER_EQUAL";
        case TOKEN_LESS:          return "TOKEN_LESS";
        case TOKEN_LESS_EQUAL:    return "TOKEN_LESS_EQUAL";
        case TOKEN_DOT:           return "TOKEN_DOT";
        case TOKEN_DOT_DOT:       return "TOKEN_DOT_DOT";
        case TOKEN_SLASH:         return "TOKEN_SLASH";
        case TOKEN_SLASH_EQUAL:   return "TOKEN_SLASH_EQUAL";

        case TOKEN_IDENTIFIER:    return "TOKEN_IDENTIFIER";
        case TOKEN_STRING:        return "TOKEN_STRING";
        case TOKEN_NUMBER:        return "TOKEN_NUMBER";
        case TOKEN_COMMENT:       return "TOKEN_COMMENT";

        case TOKEN_AND:           return "TOKEN_AND";
        case TOKEN_ELSE:          return "TOKEN_ELSE";
        case TOKEN_ELIF:          return "TOKEN_ELIF";
        case TOKEN_FALSE:         return "TOKEN_FALSE";
        case TOKEN_FOR:           return "TOKEN_FOR";
        case TOKEN_FN:            return "TOKEN_FN";
        case TOKEN_IF:            return "TOKEN_IF";
        case TOKEN_NIL:           return "TOKEN_NIL";
        case TOKEN_OR:            return "TOKEN_OR";
        case TOKEN_PRINT:         return "TOKEN_PRINT";
        case TOKEN_RETURN:        return "TOKEN_RETURN";
        case TOKEN_THIS:          return "TOKEN_THIS";
        case TOKEN_TRUE:          return "TOKEN_TRUE";
        case TOKEN_LET:           return "TOKEN_LET";
        case TOKEN_WHILE:         return "TOKEN_WHILE";
        case TOKEN_LOOP:          return "TOKEN_LOOP";
        case TOKEN_MATCH:         return "TOKEN_MATCH";

        case TOKEN_ERROR:         return "TOKEN_ERROR";
        case TOKEN_EOF:           return "TOKEN_EOF";
    }

    return "UNKNOWN_TOKEN";
}

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

    size_t read_bytes = fread(file_contents,  sizeof(char),  file_size,  file);

    fclose(file);
    return file_contents;
}

Scanner init_scanner(char* txt)
{
    Scanner scanner =
    {
        .init = (char*)txt,
        .start = txt,
        .current = txt,
        .token_list = NULL
    };
    return scanner;
}

void free_scanner(Scanner* scanner)
{
    arrfree(scanner->token_list);
    free(scanner->init);
    scanner->init = NULL;
    scanner->start = NULL;
    scanner->current = NULL;
}

bool scanner_is_at_end(Scanner scanner)
{
    return scanner.current[0] == '\0';
}


char scanner_peek(Scanner scanner)
{
    return scanner.current[0];
}

char scanner_consume(Scanner* scanner)
{
    char c = scanner_peek(*scanner);
    scanner->current++;
    return c;
}

bool scanner_match(Scanner* scanner, char to_match)
{
    if (scanner_is_at_end(*scanner)) return false;
    char peeked = scanner_peek(*scanner);
    if (peeked != to_match) return false;
    scanner->current++;
    return true;
}

void scanner_skip_whitespace(Scanner* scanner)
{
    while (true)
    {
        char c = scanner_peek(*scanner);
        switch (c)
        {
            case ' ':
            case '\t':
            case '\r':
                scanner_consume(scanner);
                scanner->column++;
                break;
            default:
                return;
        }
    }
}

Token scanner_scan_token(Scanner* scanner)
{
    scanner_skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (scanner_is_at_end(*scanner)) return scanner_make_token(scanner, TOKEN_EOF, 0, 1);

#define increment_column(col) scanner->column+=(col)
    char c = scanner_consume(scanner);
    Token rt;     // return_token
    TokenType tt; // token_type
    switch (c)
    {
        // Newline
        case '\n':
            rt =  scanner_make_token(scanner, TOKEN_NEWLINE     , 1, 0);
            scanner->column = 0;
            return rt;

        // One character tokens
        case '(': return scanner_make_token(scanner, TOKEN_LEFT_PAREN  , 0, 1);
        case ')': return scanner_make_token(scanner, TOKEN_RIGHT_PAREN , 0, 1);
        case '{': return scanner_make_token(scanner, TOKEN_LEFT_BRACE  , 0, 1);
        case '}': return scanner_make_token(scanner, TOKEN_RIGHT_BRACE , 0, 1);
        case ';': return scanner_make_token(scanner, TOKEN_SEMICOLON   , 0, 1);
        case ',': return scanner_make_token(scanner, TOKEN_COMMA       , 0, 1);
        case '.': return scanner_make_token(scanner, TOKEN_DOT         , 0, 1);
        case '-': return scanner_make_token(scanner, TOKEN_MINUS       , 0, 1);
        case '+': return scanner_make_token(scanner, TOKEN_PLUS        , 0, 1);
        case '*': return scanner_make_token(scanner, TOKEN_STAR        , 0, 1);

        // One or more character tokens
        case '/':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_SLASH_EQUAL  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_SLASH        , 0, 1);
        case '=':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_EQUAL_EQUAL  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_EQUAL        , 0, 1);
        case '<':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_LESS_EQUAL   , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_LESS         , 0, 1);
        case '>':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_GREATER_EQUAL, 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_GREATER      , 0, 1);

        // Literals
        case '#': return scanner_scan_line_comment(scanner);
        case '"': return scanner_scan_string(scanner);
        default:
            if (is_digit(c))
                    return scanner_scan_number(scanner);
    }
#undef increment_column

    return scanner_make_error_token(*scanner, "Encountered unexpected character");
}

void scanner_add_token(Scanner* scanner, Token token)
{
    arrput(scanner->token_list, token);
}

Token scanner_make_token(Scanner* scanner, TokenType token_type, int32_t lines_to_skip, int32_t columns_to_skip)
{
    Token token =
    {
        .type = token_type,
        .start = scanner->start,
        .line = scanner->line,
        .column = scanner->column,
        .length = scanner->current - scanner->start
    };
    scanner->line   += lines_to_skip;
    scanner->column += columns_to_skip;
    //fprintf(stderr, "scanner_make_token [%d:%d]: %s - %s\n", token.line, token.column, token_type_name(token.type), token.start);
    return token;
}

Token scanner_make_error_token(Scanner scanner, const char* err_msg)
{
    Token token =
    {
        .type = TOKEN_ERROR,
        .start = err_msg,
        .line = scanner.line,
        .column = scanner.column,
        .length = strlen(err_msg)
    };
    return token;
}

void scanner_scan_tokens(Scanner* scanner)
{
    while (!scanner_is_at_end(*scanner))
    {
        Token token = scanner_scan_token(scanner);
        if (token.type == TOKEN_ERROR)
        {
            fprintf(stderr, "[%d:%d]: %s\n", token.line, token.column, token.start);
            exit(1);
        }
        scanner_add_token(scanner, token);
    }
}

Token scanner_scan_string(Scanner* scanner)
{
    Token token =
    {
        .type   = TOKEN_STRING,
        .start  = scanner->start,
        .line   = scanner->line,
        .column = scanner->column,
        .length = 0
    };
    bool is_escaping = false;

    scanner->column++;
    do
    {
        char c;

        if (scanner_is_at_end(*scanner))
        {
            fprintf(stderr, "Failed to find end of string.\n");
            exit(1);
        }

        c = scanner_consume(scanner);
        scanner->column++;
        switch (c)
        {
            case '\\':
                is_escaping = is_escaping ? false : true;
                break;
            case '\n':
                scanner->line++;
                scanner->column=0;
                break;
            case '"':
                if (!is_escaping)
                {
                    token.length = scanner->current - scanner->start;
                    return token;
                }
                break;
        }

    } while(true);
}

Token scanner_scan_line_comment(Scanner* scanner)
{
    Token token =
    {
        .type   = TOKEN_COMMENT,
        .start  = scanner->start,
        .line   = scanner->line,
        .column = scanner->column,
        .length = 0
    };
    char c;

    scanner->column++;
    while (!scanner_is_at_end(*scanner) && (c = scanner_consume(scanner)) != '\n');

    scanner->column = 0;
    scanner->line++;
    token.length = scanner->current - scanner->start;
    return token;
}

bool is_digit(char c)
{
    switch(c)
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
    }
    return false;
}

bool is_alpha(char c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

Token scanner_scan_number(Scanner* scanner)
{
    Token token =
    {
        .type   = TOKEN_NUMBER,
        .start  = scanner->start,
        .line   = scanner->line,
        .column = scanner->column,
        .length = 0
    };
    char c;

    scanner->column++;
    while (!scanner_is_at_end(*scanner) && is_digit(c = scanner_peek(*scanner)))
    {
        scanner_consume(scanner);
        scanner->column++;
    }

    if (c == '.')
    {
        scanner->column++;
        scanner_consume(scanner);
        while (!scanner_is_at_end(*scanner) && is_digit(c = scanner_peek(*scanner)))
        {
            scanner_consume(scanner);
            scanner->column++;
        }
    }

    token.length = scanner->current - scanner->start;
    return token;
}

//Token scanner_scan_keyword(Scanner* scanner, const char* keyword, Token* result_token)
//{
//    size_t len = strlen(keyword);
//    if (len == 0)
//    {
//        fprintf(stderr, "Can't scan an empty keyword.");
//        exit(1);
//    }
//
//    for (size_t i = 0; i < 0; ++i)
//    {
//    }
//}
//
//Token scanner_scan_identifier(Scanner* scanner) {
//  while (is_alpha(scanner_peek(*scanner)) || is_digit(scanner_peek(*scanner)) scanner_consume(scanner);
//  return scanner_make_token(scanner_scan_identifier_type(scanner));
//}
//
//TokenType scanner_scan_identifier_type(Scanner* scanner) {
//    switch (scanner.start[0]) {
//        case 'a': return scanner_check_keyword(scanner, 1, 2, "nd"   , TOKEN_AND   );
//        case 'c': return scanner_check_keyword(scanner, 1, 4, "lass" , TOKEN_CLASS );
//        case 'i': return scanner_check_keyword(scanner, 1, 1, "f"    , TOKEN_IF    );
//        case 'n': return scanner_check_keyword(scanner, 1, 2, "il"   , TOKEN_NIL   );
//        case 'o': return scanner_check_keyword(scanner, 1, 1, "r"    , TOKEN_OR    );
//        case 'p': return scanner_check_keyword(scanner, 1, 4, "rint" , TOKEN_PRINT );
//        case 'r': return scanner_check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
//        case 'w': return scanner_check_keyword(scanner, 1, 4, "hile" , TOKEN_WHILE );
//        case 't': return scanner_check_keyword(scanner, 1, 4, "rue"  , TOKEN_TRUE );
//        case 'l': return scanner_check_keyword(scanner, 1, 4, "oop"  , TOKEN_LOOP );
//        case 'f':
//            if (scanner.current - scanner.start > 1)
//            {
//                switch (scanner.start[1])
//                {
//                  case 'a': return scanner_check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
//                  case 'o': return scanner_check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
//                  case 'n': return TOKEN_FN;
//                }
//            }
//    }
//    // else and elif are unhandled
//    //if (scanner.start[0] == 'e' and scanner.start[1] == 'l')
//    //{
//    //}
//
//    return TOKEN_IDENTIFIER;
//}
//
//TokenType scanner_check_keyword(Scanner* scanner, int start, int length, const char* rest, TokenType type) {
//  if (scanner.current - scanner.start == start + length
//     && memcmp(scanner.start + start, rest, length) == 0)
//  {
//      scanner->column += 1 + strlen(rest);
//      return type;
//  }
//
//  scanner->column += 1 + strlen(rest);
//  return TOKEN_IDENTIFIER;
//}

//Token scanner_scan_identifier(Scanner* scanner)
//{
//    Token token =
//    {
//        .type   = TOKEN_IDENTIFIER,
//        .start  = scanner->start,
//        .line   = scanner->line,
//        .column = scanner->column,
//        .length = 0
//    };
//}
