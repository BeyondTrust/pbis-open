#ifndef __GPA_LEXER_P_H__
#define __GPA_LEXER_P_H__

/*
 * Lexer states
 */
#define BEGIN_STATE    0
#define LEX_CHAR       1
#define LEX_DIGIT      2
#define LEX_SEMICOLON  3
#define LEX_COMMA      4
#define LEX_LCURBRACE  5
#define LEX_RCURBRACE  6
#define LEX_LSQBRACE   7
#define LEX_RSQBRACE   8
#define LEX_EQUALS     9
#define LEX_HYPHEN    10
#define LEX_DBL_QUOTE 11
#define LEX_EOF       12
#define LEX_OTHER     13
#define END_STATE     14

typedef struct _STATE_ENTRY {
    DWORD dwNextState;
    DWORD dwAction;
    DWORD tokenId;
} STATE_ENTRY, *PSTATE_ENTRY;

typedef struct _LEXER_STATE {
    /*
     * Path to configuration file
     */
    CHAR szFilePath[PATH_MAX+1];
    /*
     * Current line position in file
     */
    DWORD  dwLine;
    /*
     * Current column position in file
     */
    DWORD  dwCol;
    /*
     * File handle to read from
     */
    FILE* handle;
} LEXER_STATE, *PLEXER_STATE;

typedef enum {
    Consume,
    PushBack,
    Skip
} LexerAction;

#endif /* __GPA_LEXER_P_H__ */
