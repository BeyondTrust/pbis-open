#ifndef __GPA_LEXER_H__
#define __GPA_LEXER_H__

typedef enum
{
    TOKEN_LCURBRACE    =     0,
    TOKEN_RCURBRACE    =     1,
    TOKEN_LSQBRACE     =     2,
    TOKEN_RSQBRACE     =     3,
    TOKEN_IDENTIFIER   =     4,
    TOKEN_EQUALS       =     5,
    TOKEN_COMMA        =     6,
    TOKEN_SEMICOLON    =     7,
    TOKEN_NUMBER       =     8,
    TOKEN_STRING       =     9,
    TOKEN_HYPHEN       =    10,
    TOKEN_EOF          =    11
} GPTokenId;

CENTERROR
getToken(
    PGPA_TOKEN pToken
    );

CENTERROR
InitLexer(
    PSTR pszFilePath
    );
    
CENTERROR
EndLexer(
    );

#ifdef TEST
int
test_group_policy_lexer(int argc, char* argv[]);
#endif /* TEST */

#endif
