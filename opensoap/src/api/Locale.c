/*-----------------------------------------------------------------------------
 * $RCSfile: Locale.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Locale.c,v 1.7 2002/11/19 09:12:55 bandou Exp $";
#endif  /* _DEBUG */

#include "Object.h"
#include "LocaleImpl.h"

#include <OpenSOAP/Locale.h>
#include <OpenSOAP/ErrorCode.h>

#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER

#define	strcasecmp(s1, s2) _stricmp((s1), (s2))

#endif /* _MSC_VER */

const char DEFAULT_CODESET[] = "US-ASCII";

const char * const CodesetList[] = {
	DEFAULT_CODESET,
	"ISO-646-US",
	"ANSI_X3.4-1986", "ISO-IR-6", "ANSI_X3.4-1968", "ISO_646.IRV:1991",
	"ASCII", "ISO646-US", "US", "IBM367", "CP367", "csASCII"
	"C", "POSIX", "646",
	NULL,
	"Big5",
	"TCA-BIG5",
	"BIG5", "BIG5-CP950",
	"csBig5",
	"big5",
	NULL,
	"HKSCS-BIG5",
	"BIG5-HKSCS", "BIG5HKSCS", "big5hk", "big5-hkscs:unicode 3.0",
	NULL,
	"EUC-JP",
	"eucJP", "ujis", "x-eucjp", "x-euc-jp", "eucjis",
	"Extended_UNIX_Code_Packed_Format_for_Japanese", "csEUCPkdFmtJapanese",
	NULL,
	"UTF-8",
	"utf-8", "unicode-1-utf-8", "eucjputf8",
	NULL,
	"EUC-KR",
	"csEUCKR",
	"5601", "ksc-5601", "ksc-5601-1987", "ksc-5601_1987", "ksc5601",
	NULL,
	"EUC-TW",
	"cns11643", "ibm-euctw",
	NULL,
	"GB-18030",
	"GB18030", "ibm1392", "ibm-1392", "gb18030-2000",
	NULL,
	"GB2312",
	"GB-2312",
	"csGB2312", "EUC_CN", "gb2312-80", "gb2312-1980", "euccn", "euc-cn",
	NULL,
	"GB-K",
	"GBK",
	NULL,
	"ISO-8859-1",
	"ISO-IR-100", "ISO_8859-1:1987", "ISO_8859-1",
	"LATIN1", "L1",
	"latin1", "l1",
	"ibm819", "cp819",
	"IBM819", "CP819", "csISOLatin1"
	"819", "iso8859-1", "8859-1", "iso8859_1", "iso_8859_1",
	"ISO88591", 
	NULL,
	"ISO-8859-2",
	"ISO-IR-101", "ISO_8859-2:1987", "ISO_8859-2",
	"LATIN2", "L2", "csISOLatin2",
	"912", "cp912", "ibm-912", "ibm912",
	"iso8859-2", "8859-2", "iso8859_2", "iso_8859_2",
	NULL,
	"ISO-8859-3",
	"ISO-IR-109", "ISO_8859-3:1988", "ISO_8859-3",
	"LATIN3", "L3", "csISOLatin3",
	"913", "cp913", "ibm-913", "ibm913",
	"iso8859-3", "8859-3", "iso8859_3", "iso_8859_3",
	NULL,
	"ISO-8859-4",
	"ISO-IR-110", "ISO_8859-4:1988", "ISO_8859-4",
	"LATIN4", "L4", "csISOLatin4",
	"914", "cp914", "ibm-914", "ibm914",
	"iso8859-4", "8859-4", "iso8859_4", "iso_8859_4",
	NULL,
	"ISO-8859-5",
	"ISO-IR-144", "ISO_8859-5:1988", "ISO_8859-5",
	"CYRILLIC", "csISOLatinCyrillic",
	"915", "cp915", "ibm-915", "ibm915",
	"iso8859-5", "8859-5", "iso8859_5", "iso_8859_5",
	NULL,
	"ISO-8859-6",
	"ISO-IR-127", "ISO_8859-6:1987", "ISO_8859-6",
	"ECMA-114", "ASMO-708", "ARABIC", "csISOLatinArabic",
	"1089", "cp1089", "ibm-1089", "ibm1089",
	"iso8859-6", "8859-6", "iso8859_6", "iso_8859_6",
	NULL,
	"ISO-8859-7",
	"ISO-IR-126", "ISO_8859-7:1987", "ISO_8859-7",
	"ELOT_928", "ECMA-118", "greek", "greek8", "csISOLatinGreek",
	"813", "cp813", "ibm-813", "ibm813",
	"iso8859-7", "8859-7", "iso8859_7", "iso_8859_7",
	NULL,
	"ISO-8859-8",
	"ISO-IR-138", "ISO_8859-8:1988", "ISO_8859-8",
	"hebrew", "csISOLatinHebrew",
	"916", "cp916", "ibm-916", "ibm916",
	"iso8859-8", "8859-8", "iso8859_8", "iso_8859_8",
	NULL,
	"ISO-8859-9",
	"ISO-IR-148", "ISO_8859-9:1989", "ISO_8859-9",
	"latin5", "l5", "csISOLatin5",
	"920", "cp920", "ibm-920", "ibm920",
	"iso8859-9", "8859-9", "iso8859_9", "iso_8859_9",
	NULL,
	"ISO-8859-13",
	"ISO-IR-179", "LATIN7", "L7",
	"iso_8859-13", "iso8859-13", "8859-13", "iso8859_13", "iso_8859_13",
	NULL,
	"ISO-8859-14",
	"LATIN8", "L8",
	"ISO-8859-14", "iso-ir-199", "ISO_8859-14:1998", "ISO_8859-14",
	"iso-celtic",
	NULL,
	"ISO-8859-15",
	"csisolatin9", "csisolatin0", "latin9", "latin0",
	"923", "cp923", "ibm-923", "ibm923",
	"iso8859-15", "iso_8859-15", "8859-15", "iso_8859-15_FDIS", "L9",
	NULL,
	"ISO-8859-16",
	"ISO-IR-226", "LATIN10", "L10",
	NULL,
	"KOI8-R",
	"csKOI8R",
	"koi8",
	NULL,
	"KOI-8-U",
	NULL,
	"KOI-8-T",
	NULL,
	"Shift_JIS",
	"SHIFTJIS",
	"SJIS",
	"sjis",
	"MS_Kanji", "csShiftJIS",
	"pck",
	NULL,
	"VISCII",
	NULL,
	"CP-437",
	"IBM437", "CP437", "437",
	"csPC8CodePage437",
	"ibm-437",
	NULL,
	"CP-850",
	"IBM850", "cp850", "850", "csPC850Multilingual",
	"ibm-850",
	NULL,
	"CP-851",
	"IBM851", "cp851", "851", "csIBM851",
	NULL,
	"CP-852",
	"IBM852", "cp852", "852", "csPCp852",
	"ibm-852",
	NULL,
	"CP-855",
	"IBM855", "cp855", "855", "csIBM855",
	"cspcp855", "ibm-855",
	NULL,
	"CP-857",
	"IBM857", "cp857", "857", "csIBM857",
	"ibm-857",
	NULL,
	"CP-860",
	"IBM860", "cp860", "860", "csIBM860",
	"ibm-860",
	NULL,
	"CP-861",
	"IBM861", "cp861", "861", "cp-is", "csIBM861",
	"ibm-861",
	NULL,
	"CP-862",
	"IBM862", "cp862", "862", "csPC862LatinHebrew",
	"ibm-862",
	NULL,
	"CP-863",
	"IBM863", "cp863", "863", "csIBM863",
	"ibm-863",
	NULL,
	"CP-864",
	"IBM864", "cp864", "csIBM864",
	"ibm-864",
	NULL,
	"CP-865",
	"IBM865", "cp865", "865", "csIBM865",
	"ibm-865",
	NULL,
	"CP-866",
	"IBM866", "cp866", "866", "csIBM866",
	"ibm-866",
	NULL,
	"CP-868",
	"IBM868", "CP868", "cp-ar", "csIBM868",
	"ibm-868",
	NULL,
	"CP-869",
	"IBM869", "cp869", "869", "cp-gr", "csIBM869",
	NULL,
	"CP-891",
	"IBM891", "cp891", "csIBM891",
	NULL,
	"CP-903",
	"IBM903", "cp903", "csIBM903",
	NULL,
	"CP-904",
	"IBM904", "cp904", "904", "csIBM904",
	NULL,
	"CP-1251",
	"CP1251", "MS-CYRL",
	"windows-1251",
	"Cp1251",
	NULL,
	"CP-1255",
	"CP1255",
	"MS-HEBR",
	"windows-1255",
	NULL,
	"TIS-620",
	"TIS620", "TIS620-0", "TIS620.2529-1", "TIS620.2533-0", "ISO-IR-166",
	"TIS620.2533",
	NULL,
	"GEORGIAN-PS",
	NULL,
	NULL
};

extern
int
OpenSOAPLocaleGetCodesetList(/* [in]  */ const char *codeset,
							 /* [in]  */ size_t codesetLen,
							 /* [out] */ const char * const **codesetList) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (codesetList && codeset && codesetLen) {
		const char * const *listItr = CodesetList;
		while (*listItr) {
			const char * const *i = listItr;
			for (; *i; ++i) {
				const char *c = codeset;
				const char *m = *i;
				/* compare */
				for (; c != codeset + codesetLen && *m
						 && toupper(*c) == toupper(*m);
					 ++c, ++m) {
				}
				if (c == codeset + codesetLen && !*m) {
					/* match */
					break;
				}
			}
			if (*i) {
				/* found */
				break;
			}
			listItr = i + 1;
		}

		*codesetList = listItr;
		
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

static
const char *
GetMIMECodeset(/* [in] */ const char *codeset,
			   /* [in] */ size_t codesetLen) {
	const char * const *ret = CodesetList;

	OpenSOAPLocaleGetCodesetList(codeset,
								 codesetLen,
								 &ret);
	
	return *ret;
}

#if defined(NONGCC_WIN32)
#include <windows.h>

/*
=begin
= convert ACP to Codeset
=end
*/
typedef struct {
    UINT    acp;
    const char *codeset;
} ACPCodeSetMapItem;

static
ACPCodeSetMapItem ACPCodeSetMap[] = {
    {0, DEFAULT_CODESET}, /* ACP is wrong */
	{874, "TIS-620"},
    {932, "Shift_JIS"},
	{936, "GB-K"}, 
/*	{949, "EUC-KR"}, */
	{950, "Big5"},
/*	{1200, "UCS2"}, */
	{1250, "ISO-8859-2"},
	{1251, "CP-1251"},
	{1252, "ISO-8859-1"},
	{1253, "ISO-8859-7"},
	{1254, "ISO-8859-9"},
	{1255, "CP-1255"},
    {0, NULL}
};

/*
=begin
--- function#GetCurrentCodeset()
    Get current codeset.
    :Parameters
	  nothing.
    :Return Value
      :const char *
	    current codeset.
	:Note
	  This Function Is Static Function
=end
*/
static
const char *
GetCurrentCodeset(void) {
    UINT acp = GetACP();
    const ACPCodeSetMapItem *c_map_i = ACPCodeSetMap;
    const char *ret = c_map_i->codeset;

    for (; c_map_i->codeset; ++c_map_i) {
        if  (acp == c_map_i->acp) {
            ret = c_map_i->codeset;
            break;
        }
    }

    return ret;
}

#else /* !NONGCC_WIN32 */
#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#if defined(CODESET) && defined(HAVE_LANGINFO_H)

static
const char *
GetCurrentCodeset(void) {
    const char *ret = nl_langinfo(CODESET);

	if (ret) {
		ret = GetMIMECodeset(ret, strlen(ret));
	}
	if (!ret) {
		ret = DEFAULT_CODESET;
	}

    return ret;
}

#else /* !CODESET || !HAVE_LANGINFO_H */

#include <locale.h>

typedef struct {
	const char *lang;
	const char *defCodeset;
} LangDefCodesetMapItem;

static
const LangDefCodesetMapItem
LangDefCodesetMap[] = {
	{"af", "ISO-8859-1"},
	{"ar", "ISO-8859-6"},
	{"ca", "ISO-8859-1"},
	{"cs", "ISO-8859-2"},
	{"da", "ISO-8859-1"},
	{"de", "ISO-8859-1"},
	{"el", "ISO-8859-7"},
	{"en", "ISO-8859-1"},
	{"es", "ISO-8859-1"},
	{"et", "ISO-8859-1"},
	{"fi", "ISO-8859-1"},
	{"fr", "ISO-8859-1"},
	{"gl", "ISO-8859-1"},
	{"he", "ISO-8859-8"},
	{"hr", "ISO-8859-2"},
	{"hu", "ISO-8859-2"},
	{"is", "ISO-8859-1"},
	{"it", "ISO-8859-1"},
	{"ja",
#if 0 && defined(WIN32)
	 "Shift_JIS"
#else
	 "EUC-JP"
#endif
	},
	{"ko", "EUC-KR"},
	{"lt", "ISO-8859-13"},
	{"nl", "ISO-8859-1"},
	{"nn", "ISO-8859-1"},
	{"no", "ISO-8859-1"},
	{"pl", "ISO-8859-2"},
	{"pt", "ISO-8859-1"},
	{"ro", "ISO-8859-2"},
	{"ru", "KOI8-R"},
	{"sk", "ISO-8859-2"},
	{"sl", "ISO-8859-2"},
	{"sv", "ISO-8859-1"},
	{"th", "TIS-620"},
	{"tr", "ISO-8859-9"},
/*	{"zh", "BIG5"}, */
	{NULL, DEFAULT_CODESET}
};

static
const char *
GetLangDefaultCodeset(/* [in] */ const char *locale) {
	const char *ret = *CodesetList;

	if (locale) {
		const LangDefCodesetMapItem *mapItr = LangDefCodesetMap;
		
		const char *langBegin = locale;
		const char *langEnd = langBegin;
		/* langEnd search */
		for (; *langEnd && !strchr("_.@", *langEnd); ++langEnd) {
		}

		for (; mapItr->lang; ++mapItr) {
			const char *mapLang = mapItr->lang;
			const char *l = langBegin;
			/* compare */
			for (; *mapLang && l != langEnd && *l == *mapLang;
				 ++l, ++mapLang) {
			}
			if (!*mapLang) {
				/* match */
				break;
			}
		}

		ret = mapItr->defCodeset;
	}

	return ret;
}

static
const char *
GetLocalesCodeset(/* [in] */ const char *locale) {
	/* if error occured, return "US-ASCII" */
	const char *ret = *CodesetList;
	if (locale &&
		strcasecmp(locale, "C") != 0
		&& strcasecmp(locale, "POSIX") != 0) {
		const char *codesetBegin = strchr(locale, '.');
		if (!codesetBegin) {
			/* this locale has no codeset part */
			ret = GetLangDefaultCodeset(locale);
		}
		else {
			const char *codesetEnd = strchr(++codesetBegin, '@');
			if (!codesetEnd) {
				/* this locale has no modifiers part */
				codesetEnd = codesetBegin + strlen(codesetBegin);
			}
	
			ret = GetMIMECodeset(codesetBegin,
								 codesetEnd - codesetBegin);
		}
	}

	return ret;
}

/*
  Following function is wrong if setlocale call with second parameter is not ""
 */
static
const char *
GetCurrentCodeset(void) {
	const char *locale = setlocale(LC_CTYPE, NULL);
	const char *ret = GetLocalesCodeset(locale);

	return ret ? ret : DEFAULT_CODESET;
}
#endif /* !CODESET */

#endif /* !NONGCC_WIN32 */

/*
=begin
---function#OpenSOAPLocaleGetCurrentCodeset(codeset)
   Get current codeset.
   
   :Parameters
     :[out] const char ** ((|codeset|))
	   codeset return buffer.
   :Return Value
      :int
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPLocaleGetCurrentCodeset(/* [out] */ const char **codeset) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (codeset) {
		*codeset = GetCurrentCodeset();
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
---function#OpenSOAPLocaleIsCurrentCodeset(codeset, isCurrentCodeset)
   Compare current codeset.
   
   :Parameters
     :[in]  const char * ((|codeset|))
	   codeset.
	 :[out] int * ((|isCurrentCodeset|))
	   Compare result return buffer. If codeset is current codeset,
	   then *isCurrentCodeset set to non-zero, else set to zero.
   :Return Value
      :int
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPLocaleIsCurrentCodeset(/* [in]  */ const char *codeset,
							   /* [out] */ int *isCurrentCodeset) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (isCurrentCodeset) {
		ret = OPENSOAP_NO_ERROR;
		*isCurrentCodeset = 1;
		if (codeset && *codeset) {
			const char *curCodeset = NULL;
			ret = OpenSOAPLocaleGetCurrentCodeset(&curCodeset);
			if (OPENSOAP_SUCCEEDED(ret)) {
				if (strcasecmp(codeset, curCodeset) != 0) {
					*isCurrentCodeset = 0;
				}
			}
		}
	}

	return ret;
}
