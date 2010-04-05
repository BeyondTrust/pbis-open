/* csv.h */

DWORD 
CsvReset(
    PVOID pCsv
    );

DWORD
CsvOpenFile(
    PCSTR pszFilename, 
    PVOID *ppCsv
    );

DWORD
CsvGetRowCount(
    PVOID pCsv
    );

DWORD
CsvGetRow(
    PLWTCSV pCsv,
    size_t index,
    PSTR *ppszRow
    );

DWORD
CsvGetValue(
    PLWTCSV pCsv, 
    PSTR pszRow, 
    PCSTR pszFieldName,
    PSTR *ppszValue
    );

DWORD
CsvGetNext(
        PVOID pCsv,
        size_t tRow,
        PLWTDATA *ppData
        );

#if 0
static
DWORD
CsvFree(
    PSTR pszValue
    );
#endif


DWORD
CsvCloseFile(
    PVOID pCsv
    );

/* utility.c */
BOOL
StringsAreEqual(
    PCSTR,
    PCSTR
    );

BOOL
StringsNoCaseAreEqual(
    PCSTR,
    PCSTR
    );


void
LwtLogTest(
   PCSTR pszFunction,
   PCSTR pszFile,
   PCSTR pszTestDescription,
   PCSTR pszTestAPIs,
   DWORD dwError,
   PCSTR pszMsg
   );

DWORD
LwtInitLogging(
    PCSTR    pszPath,
    int      nAppend,
    int      nLogLevel
    );

VOID
LwtShutdownLogging(
    VOID
   );

DWORD
LwtMapErrorToProgramStatus(
    DWORD dwError
    );
