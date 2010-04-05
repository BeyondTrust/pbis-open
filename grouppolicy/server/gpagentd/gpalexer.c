#include "includes.h"

/*
 * global lexer state
 *
 */
LEXER_STATE gLexerState;

/*
 * BEGIN_STATE    0
 * LEX_CHAR       1
 * LEX_DIGIT      2
 * LEX_SEMICOLON  3
 * LEX_COMMA      4
 * LEX_LCURBRACE  5
 * LEX_RCURBRACE  6
 * LEX_LSQBRACE   7
 * LEX_RSQBRACE   8
 * LEX_EQUALS     9
 * LEX_HYPHEN    10
 * LEX_DBL_QUOTE 11
 * LEX_EOF       12
 * LEX_OTHER     13
 * END_STATE     14
 */

STATE_ENTRY StateTable[][14] =
{
    /* BEGIN_STATE := 0 */
    {
        {  BEGIN_STATE, Consume, -1},             /* BEGIN_STATE   */
        {     LEX_CHAR, Consume, -1},             /* LEX_CHAR      */
        {    LEX_DIGIT, Consume, -1},             /* LEX_DIGIT     */
        {    END_STATE, Consume, TOKEN_SEMICOLON},/* LEX_SEMICOLON */
        {    END_STATE, Consume, TOKEN_COMMA},    /* LEX_COMMA     */
        {    END_STATE, Consume, TOKEN_LCURBRACE},/* LEX_LCURBRACE */
        {    END_STATE, Consume, TOKEN_RCURBRACE},/* LEX_RCURBRACE */
        {    END_STATE, Consume, TOKEN_LSQBRACE}, /* LEX_LSQBRACE  */
        {    END_STATE, Consume, TOKEN_RSQBRACE}, /* LEX_RSQBRACE  */
        {    END_STATE, Consume, TOKEN_EQUALS},   /* LEX_EQUALS    */
        {    END_STATE, Consume, TOKEN_HYPHEN},   /* LEX_HYPHEN    */
        {LEX_DBL_QUOTE, Consume, -1},             /* LEX_DBL_QUOTE */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_EOF       */
        {  BEGIN_STATE, Skip,    -1}              /* LEX_OTHER     */
    },
    /* LEX_CHAR := 1 */
    {
        {  BEGIN_STATE, Consume,  -1},              /* BEGIN_STATE   */
        {     LEX_CHAR, Consume,  -1},              /* LEX_CHAR      */
        {     LEX_CHAR, Consume,  -1},              /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_IDENTIFIER},/* LEX_DBL_QUOTE */
        {    END_STATE, Skip,     TOKEN_IDENTIFIER},/* LEX_EOF       */
        {    END_STATE, Skip,     TOKEN_IDENTIFIER} /* LEX_OTHER     */
    },
    /* LEX_DIGIT := 2 */
    {
        {  BEGIN_STATE, Consume,  -1},              /* BEGIN_STATE   */
        {     LEX_CHAR, Consume,  TOKEN_NUMBER},   /* LEX_CHAR      */
        {    LEX_DIGIT, Consume,  -1},              /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_NUMBER},   /* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_NUMBER},   /* LEX_EOF       */
        {    END_STATE,     Skip, TOKEN_NUMBER}    /* LEX_OTHER     */
    },
    /* LEX_SEMICOLON := 3 */
    {
        {  BEGIN_STATE,  Consume, -1},              /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_SEMICOLON},/* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_SEMICOLON}, /* LEX_EOF       */
        {    END_STATE,  Consume, -1}               /* LEX_OTHER     */
    },
    /* LEX_COMMA := 4 */
    {
        {  BEGIN_STATE,  Consume, -1},              /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_COMMA},    /* LEX_DBL_QUOTE */
        {    END_STATE, Consume,  TOKEN_COMMA},    /* LEX_EOF       */
        {    END_STATE, Consume,  -1}              /* LEX_OTHER     */
    },
    /* LEX_LCURBRACE := 5 */
    {
        {  BEGIN_STATE, Consume,  -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_LCURBRACE},/* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_LCURBRACE},/* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_RCURBRACE := 6 */
    {
        {  BEGIN_STATE,  Consume, -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_RCURBRACE},/* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_RCURBRACE},/* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_LSQBRACE := 7 */
    {
        {  BEGIN_STATE,  Consume, -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_LSQBRACE}, /* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_LSQBRACE}, /* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_RSQBRACE := 8 */
    {
        {  BEGIN_STATE,  Consume, -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_RSQBRACE}, /* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_RSQBRACE}, /* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_EQUALS := 9 */
    {
        {  BEGIN_STATE,  Consume, -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_EQUALS},   /* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_EQUALS},   /* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_HYPHEN := 10 */
    {
        {  BEGIN_STATE,  Consume, -1},             /* BEGIN_STATE   */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_CHAR      */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_DIGIT     */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_SEMICOLON */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_COMMA     */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_LCURBRACE */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_RCURBRACE */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_LSQBRACE  */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_RSQBRACE  */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_EQUALS    */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_HYPHEN    */
        {    END_STATE, PushBack, TOKEN_HYPHEN},   /* LEX_DBL_QUOTE */
        {    END_STATE,  Consume, TOKEN_HYPHEN},   /* LEX_EOF       */
        {    END_STATE,  Consume, -1}              /* LEX_OTHER     */
    },
    /* LEX_DBL_QUOTE := 11 */
    {
        {  BEGIN_STATE, Consume, -1},              /* BEGIN_STATE   */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_CHAR      */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_DIGIT     */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_SEMICOLON */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_COMMA     */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_LCURBRACE */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_RCURBRACE */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_LSQBRACE  */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_RSQBRACE  */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_EQUALS    */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_HYPHEN    */
        {    END_STATE, Consume, TOKEN_STRING},    /* LEX_DBL_QUOTE */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_EOF       */
        {LEX_DBL_QUOTE, Consume, -1}               /* LEX_OTHER     */
    },
    /* LEX_EOF := 12 */
    {
        {  BEGIN_STATE, Consume, TOKEN_EOF},       /* BEGIN_STATE   */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_CHAR      */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_DIGIT     */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_SEMICOLON */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_COMMA     */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_LCURBRACE */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_RCURBRACE */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_LSQBRACE  */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_RSQBRACE  */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_EQUALS    */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_HYPHEN    */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_DBL_QUOTE */
        {    END_STATE, Consume, TOKEN_EOF},       /* LEX_EOF       */
        {    END_STATE, Consume, TOKEN_EOF}        /* LEX_OTHER     */
    },
    /* LEX_OTHER := 13 */
    {
        {  BEGIN_STATE, Consume, -1},              /* BEGIN_STATE   */
        {     LEX_CHAR, Consume, -1},              /* LEX_CHAR      */
        {    LEX_DIGIT, Consume, -1},              /* LEX_DIGIT     */
        {LEX_SEMICOLON, Consume, -1},              /* LEX_SEMICOLON */
        {    LEX_COMMA, Consume, -1},              /* LEX_COMMA     */
        {LEX_LCURBRACE, Consume, -1},              /* LEX_LCURBRACE */
        {LEX_RCURBRACE, Consume, -1},              /* LEX_RCURBRACE */
        { LEX_LSQBRACE, Consume, -1},              /* LEX_LSQBRACE  */
        { LEX_RSQBRACE, Consume, -1},              /* LEX_RSQBRACE  */
        {   LEX_EQUALS, Consume, -1},              /* LEX_EQUALS    */
        {   LEX_HYPHEN, Consume, -1},              /* LEX_HYPHEN    */
        {LEX_DBL_QUOTE, Consume, -1},              /* LEX_DBL_QUOTE */
        {    END_STATE, Consume, -1},              /* LEX_EOF       */
        {    LEX_OTHER, Consume, -1}               /* LEX_OTHER     */
    }
};

DWORD
FreeLexerState() {
    if ((FILE*)gLexerState.handle != NULL) {
        fclose((FILE*)gLexerState.handle);
        gLexerState.handle = NULL;
    }
    return 0;
}

CENTERROR
InitLexer(
    PSTR pszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (pszFilePath == NULL || *pszFilePath == '\0') {
        ceError = CENTERROR_INVALID_PARAMETER;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    strncpy(gLexerState.szFilePath, pszFilePath, PATH_MAX);
    *(gLexerState.szFilePath+PATH_MAX) = '\0';

    gLexerState.dwLine = 0;
    gLexerState.dwCol = 0;

    gLexerState.handle = NULL;

    gLexerState.handle = fopen(gLexerState.szFilePath, "r");
    ceError = (gLexerState.handle == NULL ? LwMapErrnoToLwError(errno) : CENTERROR_SUCCESS);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return ceError;

error:

    FreeLexerState();

    return ceError;
}

/*
 * Cleanup lexing states
 */
CENTERROR
EndLexer()
{
    return FreeLexerState();
}

DWORD
getCharacterId(
    DWORD ch
    )
{
    if (ch == EOF) {
        return LEX_EOF;
    }

    if ((ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z')) {
        return LEX_CHAR;
    }

    if (ch >= '0' && ch <= '9') {
        return LEX_DIGIT;
    }

    if (ch == ';') {
        return LEX_SEMICOLON;
    }

    if (ch == ',') {
        return LEX_COMMA;
    }

    if (ch == '{') {
        return LEX_LCURBRACE;
    }

    if (ch == '}') {
        return LEX_RCURBRACE;
    }

    if (ch == '[') {
        return LEX_LSQBRACE;
    }

    if (ch == ']') {
        return LEX_RSQBRACE;
    }

    if (ch == '"') {
        return LEX_DBL_QUOTE;
    }

    if (ch == '-') {
        return LEX_HYPHEN;
    }

    if (ch == '=') {
        return LEX_EQUALS;
    }

    return LEX_OTHER;
}

DWORD
getCharacter() {
    return getc((FILE*)gLexerState.handle);
}

DWORD
pushBackCharacter(
    BYTE ch
    )
{
    DWORD pushResult = ungetc(ch, (FILE*)gLexerState.handle);
    return (pushResult == EOF ? LwMapErrnoToLwError(errno) : CENTERROR_SUCCESS);
}

DWORD
getState(
    DWORD currentState,
    DWORD chId
    )
{
    return(StateTable[currentState][chId].dwNextState);
}

DWORD
getAction(
    DWORD currentState,
    DWORD chId
    )
{
    return(StateTable[currentState][chId].dwAction);
}

DWORD
getTokenId(
    DWORD currentState,
    DWORD chId
    )
{
    return (StateTable[currentState][chId].tokenId);
}

/*
 * Fills the given token as parsed from the file
 * The caller is responsible for allocating memory
 */
CENTERROR
getToken(
    PGPA_TOKEN pToken
    )
{
    CENTERROR ceError;
    char szToken[MAX_TOKEN_LENGTH+1];
    DWORD dwIndex = 0;
    DWORD dwToken;
    DWORD dwCurrentState = BEGIN_STATE;

    ceError = (pToken == NULL ? CENTERROR_INVALID_PARAMETER : 0);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(pToken, 0, sizeof(GPA_TOKEN));
    szToken[0] = '\0';

    do
    {
        DWORD ch = getCharacter();
        DWORD dwChId = getCharacterId(ch);
        DWORD dwState;
        DWORD dwAction;

        if (dwChId != LEX_EOF) {
            gLexerState.dwCol++;
        }

        dwState = getState(dwCurrentState, dwChId);

        if (ch == (DWORD)'\n') {
            gLexerState.dwLine++;
            gLexerState.dwCol = 0;
        }

        dwToken = getTokenId(dwCurrentState, dwChId);

        dwAction = getAction(dwCurrentState, dwChId);
        switch(dwAction)
        {
        case Skip:
        {
            ;
        }
        break;
        case Consume:
        {
            szToken[dwIndex++] = (BYTE)ch;
        }
        break;
        case PushBack:
        {
            gLexerState.dwCol--;
            ceError = pushBackCharacter((BYTE)ch);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        }

        dwCurrentState = dwState;
    } while (dwCurrentState != END_STATE);
    szToken[dwIndex] = '\0';

    pToken->dwToken = dwToken;
    pToken->dwLength = dwIndex;
    strncpy(pToken->szString, szToken, MAX_TOKEN_LENGTH);
    *(pToken->szString+MAX_TOKEN_LENGTH+1) = '\0';

    return ceError;

error:

    return ceError;
}

#ifdef TEST
void
print_token(
    PGPA_TOKEN pToken
    )
{
    if (pToken == NULL) {
        return;
    }

    printf("Token type: ");
    switch (pToken->dwToken)
    {
    case TOKEN_LCURBRACE:
    {
        printf("TOKEN_LCURBRACE\n");
    }
    break;
    case TOKEN_RCURBRACE:
    {
        printf("TOKEN_RCURBRACE\n");
    }
    break;
    case TOKEN_LSQBRACE:
    {
        printf("TOKEN_LSQBRACE\n");
    }
    break;
    case TOKEN_RSQBRACE:
    {
        printf("TOKEN_RSQBRACE\n");
    }
    break;
    case TOKEN_IDENTIFIER:
    {
        printf("TOKEN_IDENTIFIER\n");
    }
    break;
    case TOKEN_EQUALS:
    {
        printf("TOKEN_EQUALS\n");
    }
    break;
    case TOKEN_COMMA:
    {
        printf("TOKEN_COMMA\n");
    }
    break;
    case TOKEN_SEMICOLON:
    {
        printf("TOKEN_SEMICOLON\n");
    }
    break;
    case TOKEN_NUMBER:
    {
        printf("TOKEN_NUMBER\n");
    }
    break;
    case TOKEN_STRING:
    {
        printf("TOKEN_STRING\n");
    }
    break;
    case TOKEN_HYPHEN:
    {
        printf("TOKEN_HYPHEN\n");
    }
    break;
    case TOKEN_EOF:
    {
        printf("TOKEN_EOF\n");
    }
    break;
    }

    if (pToken->dwToken != TOKEN_EOF) {
        printf("Token length: %ld\n", pToken->dwLength);
        if (pToken->szString != NULL && *pToken->szString != '\0') {
            printf("Token content: [%s]\n", pToken->szString);
        }
        else {
            printf("Token content: NULL\n");
        }
    }
}

CENTERROR
test_group_policy_lexer(
    int argc,
    char* argv[]
    )
{
    CENTERROR ceError;
    PSTR pszFilePath;
    GPA_TOKEN token;

    if (argc < 2) {
        printf("Usage: gpagent <filename>\n");
        return 1;
    }

    pszFilePath = argv[1];
    if (pszFilePath == NULL || *pszFilePath == '\0') {
        printf("An invalid config filename was specified.\n");
        return 1;
    }

    printf("Config filename : %s\n", pszFilePath);

    ceError = InitLexer(pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    do {
        ceError = getToken(&token);
        BAIL_ON_CENTERIS_ERROR(ceError);

        print_token(&token);
    } while (token.dwToken != TOKEN_EOF);

    EndLexer();

    return ceError;

error:

    EndLexer();

    printf("%s exiting with error [Code:%ld]\n", argv[0], ceError);

    return ceError;
}

#endif
