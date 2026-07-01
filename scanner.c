#include "scanner.h"

const char* token_type_name(TokenType type)
{
    switch (type)
    {
        // Single-character tokens.
        case TOKEN_LEFT_PAREN:    return "TOKEN_LEFT_PAREN";
        case TOKEN_RIGHT_PAREN:   return "TOKEN_RIGHT_PAREN";
        case TOKEN_LEFT_BRACE:    return "TOKEN_LEFT_BRACE";
        case TOKEN_RIGHT_BRACE:   return "TOKEN_RIGHT_BRACE";
        case TOKEN_LEFT_SQUARE:   return "TOKEN_LEFT_SQUARE";
        case TOKEN_RIGHT_SQUARE:  return "TOKEN_RIGHT_SQUARE";
        case TOKEN_PIPE:          return "TOKEN_PIPE";
        case TOKEN_COMMA:         return "TOKEN_COMMA";
        case TOKEN_SEMICOLON:     return "TOKEN_SEMICOLON";
        case TOKEN_UNDERSCORE:    return "TOKEN_UNDERSCORE";
        case TOKEN_NEWLINE:       return "TOKEN_NEWLINE";

        // One or two character tokens.
        case TOKEN_BANG:            return "TOKEN_BANG";
        case TOKEN_BANG_EQUAL:      return "TOKEN_BANG_EQUAL";

        case TOKEN_EQUAL:           return "TOKEN_EQUAL";
        case TOKEN_EQUAL_EQUAL:     return "TOKEN_EQUAL_EQUAL";
        case TOKEN_EQUAL_GREATER:   return "TOKEN_EQUAL_GREATER";

        case TOKEN_LESS:            return "TOKEN_LESS";
        case TOKEN_LESS_EQUAL:      return "TOKEN_LESS_EQUAL";

        case TOKEN_GREATER:         return "TOKEN_GREATER";
        case TOKEN_GREATER_EQUAL:   return "TOKEN_GREATER_EQUAL";

        case TOKEN_DOT:             return "TOKEN_DOT";
        case TOKEN_DOT_DOT:         return "TOKEN_DOT_DOT";

        case TOKEN_COLON:           return "TOKEN_COLON";
        case TOKEN_COLON_COLON:     return "TOKEN_COLON_COLON";

        case TOKEN_PLUS:            return "TOKEN_PLUS";
        case TOKEN_PLUS_EQUAL:      return "TOKEN_PLUS_EQUAL";
        case TOKEN_PLUS_PLUS:       return "TOKEN_PLUS_PLUS";

        case TOKEN_MINUS:           return "TOKEN_MINUS";
        case TOKEN_MINUS_EQUAL:     return "TOKEN_MINUS_EQUAL";
        case TOKEN_MINUS_MINUS:     return "TOKEN_MINUS_MINUS";

        case TOKEN_STAR:            return "TOKEN_STAR";
        case TOKEN_STAR_EQUAL:      return "TOKEN_STAR_EQUAL";

        case TOKEN_SLASH:           return "TOKEN_SLASH";
        case TOKEN_SLASH_EQUAL:     return "TOKEN_SLASH_EQUAL";

        case TOKEN_PERCENT:         return "TOKEN_PERCENT";
        case TOKEN_PERCENT_EQUAL:   return "TOKEN_PERCENT_EQUAL";

        // Literals.
        case TOKEN_IDENTIFIER:      return "TOKEN_IDENTIFIER";
        case TOKEN_STRING:          return "TOKEN_STRING";
        case TOKEN_INTEGER:         return "TOKEN_INTEGER";
        case TOKEN_NUMBER:          return "TOKEN_NUMBER";
        case TOKEN_COMMENT:         return "TOKEN_COMMENT";

        // Keywords.
        case TOKEN_NIL_T:           return "TOKEN_NIL_T";
        case TOKEN_BOOL:            return "TOKEN_BOOL";
        case TOKEN_INT:             return "TOKEN_INT";
        case TOKEN_REAL:            return "TOKEN_REAL";
        case TOKEN_LET:             return "TOKEN_LET";
        case TOKEN_ALIAS:           return "TOKEN_ALIAS";
        case TOKEN_TYPE:            return "TOKEN_TYPE";
        case TOKEN_EFFECT:          return "TOKEN_EFFECT";
        case TOKEN_NIL_V:           return "TOKEN_NIL_V";
        case TOKEN_TRUE:            return "TOKEN_TRUE";
        case TOKEN_FALSE:           return "TOKEN_FALSE";
        case TOKEN_NOT:             return "TOKEN_NOT";
        case TOKEN_AND:             return "TOKEN_AND";
        case TOKEN_OR:              return "TOKEN_OR";
        case TOKEN_DO:              return "TOKEN_DO";
        case TOKEN_END:             return "TOKEN_END";
        case TOKEN_IF:              return "TOKEN_IF";
        case TOKEN_ELIF:            return "TOKEN_ELIF";
        case TOKEN_ELSE:            return "TOKEN_ELSE";
        case TOKEN_WHILE:           return "TOKEN_WHILE";
        case TOKEN_FOR:             return "TOKEN_FOR";
        case TOKEN_IN:              return "TOKEN_IN";
        case TOKEN_BREAK:           return "TOKEN_BREAK";
        case TOKEN_LOOP:            return "TOKEN_LOOP";
        case TOKEN_CONTINUE:        return "TOKEN_CONTINUE";
        case TOKEN_CTL:             return "TOKEN_CTL";
        case TOKEN_FN:              return "TOKEN_FN";
        case TOKEN_RETURN:          return "TOKEN_RETURN";
        case TOKEN_MATCH:           return "TOKEN_MATCH";
        case TOKEN_HANDLE:          return "TOKEN_HANDLE";
        case TOKEN_PRINT:           return "TOKEN_PRINT";

        // Special.
        case TOKEN_ERROR:           return "TOKEN_ERROR";
        case TOKEN_EOF:             return "TOKEN_EOF";
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

    fread(file_contents,  sizeof(char),  file_size,  file);

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
    Token rt;     // return_token
    char c = scanner_consume(scanner);
    switch (c)
    {
        // Newline
        case '\n':
            rt =  scanner_make_token(scanner, TOKEN_NEWLINE     , 1, 0);
            scanner->column = 0;
            return rt;

        //TOKEN_BANG      , TOKEN_BANG_EQUAL   ,
        //TOKEN_EQUAL     , TOKEN_EQUAL_EQUAL  , TOKEN_EQUAL_GREATER,
        //TOKEN_LESS      , TOKEN_LESS_EQUAL   ,
        //TOKEN_GREATER   , TOKEN_GREATER_EQUAL,
        //TOKEN_DOT       , TOKEN_DOT_DOT      ,
        //TOKEN_COLON     , TOKEN_COLON_COLON  ,
        //TOKEN_PLUS      , TOKEN_PLUS_EQUAL   ,
        //TOKEN_MINUS     , TOKEN_MINUS_EQUAL  ,
        //TOKEN_STAR      , TOKEN_STAR_EQUAL   ,
        //TOKEN_SLASH     , TOKEN_SLASH_EQUAL  ,
        //TOKEN_PERCENT   , TOKEN_PERCENT_EQUAL,

        // One character tokens
        case '(': return scanner_make_token(scanner, TOKEN_LEFT_PAREN  , 0, 1);
        case ')': return scanner_make_token(scanner, TOKEN_RIGHT_PAREN , 0, 1);
        case '{': return scanner_make_token(scanner, TOKEN_LEFT_BRACE  , 0, 1);
        case '}': return scanner_make_token(scanner, TOKEN_RIGHT_BRACE , 0, 1);
        case '[': return scanner_make_token(scanner, TOKEN_LEFT_SQUARE , 0, 1);
        case ']': return scanner_make_token(scanner, TOKEN_RIGHT_SQUARE, 0, 1);
        case '|': return scanner_make_token(scanner, TOKEN_PIPE        , 0, 1);
        case ',': return scanner_make_token(scanner, TOKEN_COMMA       , 0, 1);
        case ';': return scanner_make_token(scanner, TOKEN_SEMICOLON   , 0, 1);

        // One or more character tokens
        case '!':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_BANG_EQUAL   , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_BANG         , 0, 1);
        case '=':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_EQUAL_EQUAL  , 0, 2);
            else if (scanner_match(scanner, '>'))
                return scanner_make_token(scanner, TOKEN_EQUAL_GREATER, 0, 1);
            else
                return scanner_make_token(scanner, TOKEN_EQUAL        , 0, 1);
        case '+':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_PLUS_EQUAL  , 0, 2);
            else if (scanner_match(scanner, '+'))
                return scanner_make_token(scanner, TOKEN_PLUS_PLUS   , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_PLUS        , 0, 1);
        case '-':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_MINUS_EQUAL  , 0, 2);
            else if (scanner_match(scanner, '-'))
                return scanner_make_token(scanner, TOKEN_MINUS_MINUS  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_MINUS        , 0, 1);
        case '*':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_STAR_EQUAL  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_STAR        , 0, 1);
        case '/':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_SLASH_EQUAL  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_SLASH        , 0, 1);
        case '%':
            if (scanner_match(scanner, '='))
                return scanner_make_token(scanner, TOKEN_PERCENT_EQUAL, 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_PERCENT      , 0, 1);
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
        case '.':
            if (scanner_match(scanner, '.'))
                return scanner_make_token(scanner, TOKEN_DOT_DOT      , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_DOT          , 0, 1);
        case ':':
            if (scanner_match(scanner, ':'))
                return scanner_make_token(scanner, TOKEN_COLON_COLON  , 0, 2);
            else
                return scanner_make_token(scanner, TOKEN_COLON        , 0, 1);

        // Literals
        case '#': return scanner_scan_line_comment(scanner);
        case '"': return scanner_scan_string(scanner);
        default:
            if (is_digit(c))
                    return scanner_scan_number(scanner);
            else if (is_alpha(c))
            {
                switch (c)
                {
                    case 'N': // Nil
                        if (scanner_match_string(scanner, "il", 1))
                            rt = scanner_make_token(scanner, TOKEN_NIL_T, 0, 2);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'B': // Bool
                        if (scanner_match_string(scanner, "ool", 1))
                            rt = scanner_make_token(scanner, TOKEN_BOOL, 0, 3);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'I': // Int
                        if (scanner_match_string(scanner, "nt", 1))
                            rt = scanner_make_token(scanner, TOKEN_INT, 0, 2);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'R': // Real
                        if (scanner_match_string(scanner, "eal", 1))
                            rt = scanner_make_token(scanner, TOKEN_REAL, 0, 3);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'l': // let, loop
                        if (scanner_match_string(scanner, "et", 1))
                            rt = scanner_make_token(scanner, TOKEN_LET, 0, 2);
                        else if (scanner_match_string(scanner, "oop", 1))
                            rt = scanner_make_token(scanner, TOKEN_LOOP, 0, 3);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 't': // type, true
                        if (scanner_match_string(scanner, "ype", 1))
                            rt = scanner_make_token(scanner, TOKEN_TYPE, 0, 3);
                        else if (scanner_match_string(scanner, "rue", 1))
                            rt = scanner_make_token(scanner, TOKEN_TRUE, 0, 3);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'e': // effect, end, elif, else
                        if (scanner_match_string(scanner, "ffect", 1))
                            rt = scanner_make_token(scanner, TOKEN_EFFECT, 0, 5);
                        else if (scanner_match_string(scanner, "nd", 1))
                            rt = scanner_make_token(scanner, TOKEN_END, 0, 2);
                        else if (scanner_match_string(scanner, "lif", 1))
                            rt = scanner_make_token(scanner, TOKEN_ELIF, 0, 3);
                        else if (scanner_match_string(scanner, "lse", 1))
                            rt = scanner_make_token(scanner, TOKEN_ELSE, 0, 3);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'n': // nil
                        if (scanner_match_string(scanner, "il", 1))
                            rt = scanner_make_token(scanner, TOKEN_NIL_V, 0, 2);
                        else if (scanner_match_string(scanner, "ot", 1))
                            rt = scanner_make_token(scanner, TOKEN_NOT, 0, 2);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'f': // false, for, fn
                        if (scanner_match_string(scanner, "alse", 1))
                            rt = scanner_make_token(scanner, TOKEN_FALSE, 0, 4);
                        else if (scanner_match_string(scanner, "or", 1))
                            rt = scanner_make_token(scanner, TOKEN_FOR, 0, 2);
                        else if (scanner_match_string(scanner, "n", 1))
                            rt = scanner_make_token(scanner, TOKEN_FN, 0, 1);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'a': // and, alias
                        if (scanner_match_string(scanner, "nd", 1))
                            rt = scanner_make_token(scanner, TOKEN_AND, 0, 2);
                        else if (scanner_match_string(scanner, "lias", 1))
                            rt = scanner_make_token(scanner, TOKEN_ALIAS, 0, 4);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'o': // or
                        if (scanner_match_string(scanner, "r", 1))
                            rt = scanner_make_token(scanner, TOKEN_OR, 0, 1);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'd': // do
                        if (scanner_match_string(scanner, "o", 1))
                            rt = scanner_make_token(scanner, TOKEN_DO, 0, 1);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'i': // if, in
                        if (scanner_match_string(scanner, "f", 1))
                            rt = scanner_make_token(scanner, TOKEN_IF, 0, 1);
                        else if (scanner_match_string(scanner, "n", 1))
                            rt = scanner_make_token(scanner, TOKEN_IN, 0, 1);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'w': // while
                        if (scanner_match_string(scanner, "hile", 1))
                            rt = scanner_make_token(scanner, TOKEN_WHILE, 0, 4);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'b': // break
                        if (scanner_match_string(scanner, "reak", 1))
                            rt = scanner_make_token(scanner, TOKEN_BREAK, 0, 4);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'c': // ctl, continue
                        if (scanner_match_string(scanner, "tl", 1))
                            rt = scanner_make_token(scanner, TOKEN_CTL, 0, 2);
                        else if (scanner_match_string(scanner, "ontinue", 1))
                            rt = scanner_make_token(scanner, TOKEN_CONTINUE, 0, 7);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'r': // return
                        if (scanner_match_string(scanner, "eturn", 1))
                            rt = scanner_make_token(scanner, TOKEN_RETURN, 0, 5);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'm': // match
                        if (scanner_match_string(scanner, "atch", 1))
                            rt = scanner_make_token(scanner, TOKEN_MATCH, 0, 4);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'h': // handle
                        if (scanner_match_string(scanner, "andle", 1))
                            rt = scanner_make_token(scanner, TOKEN_HANDLE, 0, 5);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    case 'p': // print
                        if (scanner_match_string(scanner, "rint", 1))
                            rt = scanner_make_token(scanner, TOKEN_PRINT, 0, 4);
                        else
                            rt = scanner_scan_identifier(scanner);
                        break;

                    default:
                        rt = scanner_scan_identifier(scanner);
                        break;
                }

                scanner->column++;
                return rt;
            }
            // I can already tell that this is going to cause some issues later down the road...
            // But today is not tomorrow, so that'll be the problem of future me :D
            else if (c == '_')
            {
                char c = scanner_peek(*scanner);
                if (is_identifier_middle(c) || is_identifier_end(c))
                {
                    rt = scanner_scan_identifier(scanner);
                    scanner->column++;
                    return rt;
                }
                else
                    return scanner_make_token(scanner, TOKEN_UNDERSCORE, 0, 1);
            }
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
        .type   = TOKEN_INTEGER,
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

    if (c == '.' && is_digit(scanner_peek(*scanner)))
    {
        token.type = TOKEN_NUMBER;
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

bool scanner_match_string(Scanner* scanner, const char* str, int32_t already_scanned)
{
    int32_t len = strlen(str);
    if (len == 0) return false;

    for (size_t i = 0; i < strlen(str); ++i)
    {
        char c = str[i];
        if (c == scanner_peek(*scanner))
            scanner_consume(scanner);
        else
        {
            scanner->current = scanner->start + already_scanned;
            return false;
        }
    }

    scanner->column += len;
    return true;
}

bool is_identifier_middle(char c)
{
    if (is_alpha(c) || c == '_' || c == '-' || is_digit(c))
        return true;
    return false;
}

bool is_identifier_end(char c)
{
    if (is_alpha(c) || c == '_' || c == '?')
        return true;
    return false;
}

Token scanner_scan_identifier(Scanner* scanner)
{
    Token token =
    {
        .type   = TOKEN_IDENTIFIER,
        .start  = scanner->start,
        .line   = scanner->line,
        .column = scanner->column,
        .length = 0
    };

    // Perfectly valid identifier name.
    //_------1212121212-?
    while (true)
    {
        if (is_identifier_middle(scanner_peek(*scanner)))
        {
            scanner_consume(scanner);
            scanner->column++;
        }
        else if (is_identifier_end(scanner_peek(*scanner)))
        {
            scanner_consume(scanner);
            scanner->column++;
            break;
        }
        else break;
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

bool is_newline(TokenType type)
{
    switch (type)
    {
        case TOKEN_SEMICOLON: return true;
        case TOKEN_NEWLINE  : return true;
        default: return false;
    }
}

