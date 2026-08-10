#include "token.h"

const char* show_token_type(TokenType type)
{
    switch (type)
    {
        // Single-character tokens.
        case TOKEN_LEFT_PAREN  : return "TOKEN_LEFT_PAREN"  ;
        case TOKEN_RIGHT_PAREN : return "TOKEN_RIGHT_PAREN" ;
        case TOKEN_LEFT_BRACE  : return "TOKEN_LEFT_BRACE"  ;
        case TOKEN_RIGHT_BRACE : return "TOKEN_RIGHT_BRACE" ;
        case TOKEN_LEFT_SQUARE : return "TOKEN_LEFT_SQUARE" ;
        case TOKEN_RIGHT_SQUARE: return "TOKEN_RIGHT_SQUARE";
        case TOKEN_PIPE        : return "TOKEN_PIPE"        ;
        case TOKEN_COMMA       : return "TOKEN_COMMA"       ;
        case TOKEN_SEMICOLON   : return "TOKEN_SEMICOLON"   ;
        case TOKEN_UNDERSCORE  : return "TOKEN_UNDERSCORE"  ;
        case TOKEN_NEWLINE     : return "TOKEN_NEWLINE"     ;

        // One or two character tokens.
        case TOKEN_BANG         : return "TOKEN_BANG"         ;
        case TOKEN_BANG_EQUAL   : return "TOKEN_BANG_EQUAL"   ;
        case TOKEN_EQUAL        : return "TOKEN_EQUAL"        ;
        case TOKEN_EQUAL_EQUAL  : return "TOKEN_EQUAL_EQUAL"  ;
        case TOKEN_EQUAL_GREATER: return "TOKEN_EQUAL_GREATER";
        case TOKEN_LESS         : return "TOKEN_LESS"         ;
        case TOKEN_LESS_EQUAL   : return "TOKEN_LESS_EQUAL"   ;
        case TOKEN_GREATER      : return "TOKEN_GREATER"      ;
        case TOKEN_GREATER_EQUAL: return "TOKEN_GREATER_EQUAL";
        case TOKEN_DOT          : return "TOKEN_DOT"          ;
        case TOKEN_DOT_DOT      : return "TOKEN_DOT_DOT"      ;
        case TOKEN_COLON        : return "TOKEN_COLON"        ;
        case TOKEN_COLON_COLON  : return "TOKEN_COLON_COLON"  ;
        case TOKEN_PLUS         : return "TOKEN_PLUS"         ;
        case TOKEN_PLUS_EQUAL   : return "TOKEN_PLUS_EQUAL"   ;
        case TOKEN_PLUS_PLUS    : return "TOKEN_PLUS_PLUS"    ;
        case TOKEN_MINUS        : return "TOKEN_MINUS"        ;
        case TOKEN_MINUS_EQUAL  : return "TOKEN_MINUS_EQUAL"  ;
        case TOKEN_MINUS_MINUS  : return "TOKEN_MINUS_MINUS"  ;
        case TOKEN_MINUS_GREATER: return "TOKEN_MINUS_GREATER";
        case TOKEN_STAR         : return "TOKEN_STAR"         ;
        case TOKEN_STAR_EQUAL   : return "TOKEN_STAR_EQUAL"   ;
        case TOKEN_SLASH        : return "TOKEN_SLASH"        ;
        case TOKEN_SLASH_EQUAL  : return "TOKEN_SLASH_EQUAL"  ;
        case TOKEN_PERCENT      : return "TOKEN_PERCENT"      ;
        case TOKEN_PERCENT_EQUAL: return "TOKEN_PERCENT_EQUAL";

        // Literals.
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_STRING    : return "TOKEN_STRING"    ;
        case TOKEN_INTEGER   : return "TOKEN_INTEGER"   ;
        case TOKEN_NUMBER    : return "TOKEN_NUMBER"    ;
        case TOKEN_COMMENT   : return "TOKEN_COMMENT"   ;

        // Keywords.
        case TOKEN_NIL_T   : return "TOKEN_NIL_T"   ;
        case TOKEN_BOOL    : return "TOKEN_BOOL"    ;
        case TOKEN_INT     : return "TOKEN_INT"     ;
        case TOKEN_NAT     : return "TOKEN_NAT"     ;
        case TOKEN_REAL    : return "TOKEN_REAL"    ;
        case TOKEN_LET     : return "TOKEN_LET"     ;
        case TOKEN_ALIAS   : return "TOKEN_ALIAS"   ;
        case TOKEN_TYPE    : return "TOKEN_TYPE"    ;
        case TOKEN_NIL_V   : return "TOKEN_NIL_V"   ;
        case TOKEN_TRUE    : return "TOKEN_TRUE"    ;
        case TOKEN_FALSE   : return "TOKEN_FALSE"   ;
        case TOKEN_NOT     : return "TOKEN_NOT"     ;
        case TOKEN_AND     : return "TOKEN_AND"     ;
        case TOKEN_OR      : return "TOKEN_OR"      ;
        case TOKEN_DO      : return "TOKEN_DO"      ;
        case TOKEN_END     : return "TOKEN_END"     ;
        case TOKEN_IF      : return "TOKEN_IF"      ;
        case TOKEN_ELIF    : return "TOKEN_ELIF"    ;
        case TOKEN_ELSE    : return "TOKEN_ELSE"    ;
        case TOKEN_WHILE   : return "TOKEN_WHILE"   ;
        case TOKEN_FOR     : return "TOKEN_FOR"     ;
        case TOKEN_IN      : return "TOKEN_IN"      ;
        case TOKEN_BREAK   : return "TOKEN_BREAK"   ;
        case TOKEN_LOOP    : return "TOKEN_LOOP"    ;
        case TOKEN_CONTINUE: return "TOKEN_CONTINUE";
        case TOKEN_FN      : return "TOKEN_FN"      ;
        case TOKEN_RETURN  : return "TOKEN_RETURN"  ;
        case TOKEN_MATCH   : return "TOKEN_MATCH"   ;

        // Special.
        case TOKEN_WHITESPACE: return "TOKEN_WHITESPACE";
        case TOKEN_ERROR     : return "TOKEN_ERROR"     ;
        case TOKEN_EOF       : return "TOKEN_EOF"       ;
    }

    return "UNKNOWN_TOKEN";
}
