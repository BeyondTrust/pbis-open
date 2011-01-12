/*-----------------------------------------------------------------------------
 * $RCSfile: String.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: String.c,v 1.128 2003/06/30 08:37:10 okada Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/Locale.h>
#include "StringImpl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#if defined(HAVE_WCHAR_H) || defined(NONGCC_WIN32)
#include <wchar.h>
#endif

#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#define vswprintf _vsnwprintf

#define HAVE_VSWPRINTF 1

#define wmemmove(dest, src, n) \
(wchar_t *)memmove((dest), (src), (n) * sizeof(wchar_t))
#define wmemcpy(dest, src, n) \
(wchar_t *)memcpy((dest), (src), (n) * sizeof(wchar_t))

/* strcasecmp definition */
#define	strcasecmp(s1, s2) _stricmp((s1), (s2))

#else /* !_MSC_VER */

#if !defined(HAVE_WCSCMP)
extern
int
wcscmp(const wchar_t *s1,
       const wchar_t *s2) {
    int ret = 0;
    if (s1 && s2) {
        for (; (ret = (long)(*s1) - (long)(*s2)) == 0 && *s1 && *s2;
             ++s1, ++s2) {
        }
    }
    
    return ret;
}
#endif /* !HAVE_WCSCMP */

#if !defined(HAVE_WCSCPY)

extern
wchar_t *
wcscpy(wchar_t *dest,
       const wchar_t *src) {
    if (dest && src) {
        wchar_t *d = dest;
        while (*d++ = *src++) {
        }
    }
    
    return dest;
}

#endif /* !HAVE_WCSCPY */

#if !defined(HAVE_WCSLEN)

extern
size_t
wcslen(const wchar_t *s) {
    const wchar_t *d = s;
    if (s) {
        for (; *d; ++d) {
        }
    }
    
    return d - s;
}

#endif /* !HAVE_WCSLEN */

#if !defined(HAVE_WCSNCAT)

extern
wchar_t *
wcsncat(wchar_t *dest,
        const wchar_t *src,
        size_t n) {
    wchar_t *d = dest;
    if (src && dest) {
        /* dest term search */
        for (; *d; ++d) {
        }
        /* copy src term or n size */
        for (; *src && n; --n) {
            *d++ = *src++;
        }
        *d = L'\0';
    }
    
    return dest;
}

#endif /* !HAVE_WCSNCAT */

#if !defined(HAVE_WCSSTR)

extern
wchar_t *
wcsstr(const wchar_t *heystack,
       const wchar_t *needle) {
    wchar_t *ret = (wchar_t *)heystack;

    if (ret && needle) {
        const wchar_t *n_end = needle;
        for (; *ret && *n_end; ++ret) {
            if (*n_end == *ret) {
                ++n_end;
            }
            else {
                n_end = needle;
            }
        }
        if (*n_end) {
            ret = NULL;
        }
        else {
            ret -= (n_end - needle);
        }
    }
    
    return ret;
}

#endif /* !HAVE_WCSSTR */

#if !defined(HAVE_WMEMCPY)

extern
wchar_t *
wmemcpy(wchar_t *dest,
        const wchar_t *src,
        size_t n) {
    if (dest && src) {
        wchar_t *d = dest;
        for (; n; --n) {
            *d++ = *src++;
        }
    }
    
    return dest;
}

#endif /* !HAVE_WMEMCPY */

#if !defined(HAVE_WMEMMOVE)

extern
wchar_t *
wmemmove(wchar_t *dest,
         const wchar_t *src,
         size_t n) {
    if (dest && src) {
        wchar_t *d = dest;
        if (dest > src) {
            const wchar_t *s = src + n;
            d += n;
            for (; n; --n) {
                *(--d) = *(--s);
            }
        }
        else if (dest < src) {
            for (; n; --n) {
                *d++ = *src++;
            }
        }
    }
    
    return dest;
}

#endif /* !HAVE_WMEMMOVE */

#endif /* _MSC_VER */


#if defined(WITH_MBFUNCS)
/*
  temporary implemantation
 */
static
int
sblen(const char *s,
      size_t n) {
    int ret = 0;

    if (s) {
		ret = -1;
		if (n) {
			if (!*s) {
				ret = 0;
			}
			else if (!(*s & 0x80)) {
				ret = 1;
			}
		}
    }

    return ret;
}

static
size_t
wcstosbs(char *dest,
         const wchar_t *src,
         size_t n) {
    size_t ret = (size_t)(-1);
    if (src) {
        const wchar_t *i = src;
        const wchar_t *e = src + n;
        for (; *i && (*i < 0x80); ++i) {
            if (dest) {
                if (i == e) {
                    break;
                }
                *dest++ = (char)*i;
            }
        }
        if (i == e || !*i) {
            if (dest) {
                *dest = '\0';
            }
            ret = i - src;
        }
    }

    return ret;
}

static
size_t
sbstowcs(wchar_t *dest,
         const char *src,
         size_t n) {
    size_t ret = (size_t)(-1);
    if (src) {
        const char *i = src;
        const char *e = src + n;
        for (; *i && !(*i & 0x80); ++i) {
            if (dest) {
                if (i == e) {
                    break;
                }
                *dest++ = (wchar_t)*i;
            }
        }
        if (i == e || !*i) {
            if (dest) {
                *dest = L'\0';
            }
            ret = i - src;
        }
    }

    return ret;
}

#define mblen(s, n) sblen((s), (n))
#define mbstowcs(dest, src, n) sbstowcs((dest), (src), (n))
#define wcstombs(dest, src, n) wcstosbs((dest), (src), (n))

#endif

/*
=begin
= String.c
OpenSOAPString Implement file.
=end
*/

/*
=begin
--- function#XwcsFree(wcstr)
    wchar_t's string free and pointer set to NULL.
    :Parameters
      :[in, out] wchar_t ** ((|wcstr|))
        Wide Character string's pointer.
    :Return Value
      nothing.
=end
 */
static
void
XwcsFree(/* [in, out] */ wchar_t **wcstr) {
    if (wcstr) {
        free(*wcstr);
        *wcstr = NULL;
    }
}

/*
=begin
--- function#XwcsRealloc(wcstr, wcstrSize)
    Realloc wchar_t's string.
    :Parameters
      :[in] wchar_t *((|wcstr|))
        Wide character string pointer.
      :[in] size_t ((|wcstrSize|))
        Wide Character string buffer size (include L'\0').
    :Return Value
      :wchar_t *
        allocatedbuffer
=end
 */
static
wchar_t *
XwcsRealloc(/* [in] */ wchar_t *wcstr,
            /* [in] */ size_t wcstrSize) {
    wchar_t *ret = NULL;

    if (wcstrSize) {
        ret = realloc(wcstr, wcstrSize * sizeof(wchar_t));
    }

    return ret;
}

/*
=begin
--- function#XwcsAlloc(wcstrSize)
    Allocate wchar_t's string.
	
    :Parameters
      :[in] size_t ((|wcstrSize|))
        Wide Character string buffer size (include L'\0')
    :Return Value
      :wchar_t *
        allocatedbuffer
=end
 */
static
wchar_t *
XwcsAlloc(/* [in] */ size_t wcstrSize) {
	return XwcsRealloc(0, wcstrSize);
}

/*
=begin
--- function#Xmbstowcs(mbstr, sz)
    Free wchar_t's string.
	
    :Parameters
      :[in] const char *((|mbstr|))
        Multi Byte string
      :[in, out] size_t *((|mbs_sz|))
        [in] Multi Byte string byte size
        [out] Wide Character length
      :[out] wchar_t **((|wcs|))
        output wchar
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
Xmbstowcs(/* [in]      */ const char *mbstr,
          /* [in, out] */ size_t *sz,
          /* [out]     */ wchar_t **wcs) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (wcs && mbstr) {
        size_t dummy = 0;
        size_t wcsLen = -1;
        if (!sz) {
            sz = &dummy;
        }
        if (*sz) {
			int d = 0;
			int n = *sz;
			const char *mbI = mbstr;
            mblen(NULL, 0); /* clear mbstate */
			for (wcsLen = 0;
				 (n > 0) && ((d = mblen(mbI, n)) != -1) && (d != 0);
				 mbI += d, n -= d, ++wcsLen) {
			}
			if (d == -1) {
				wcsLen = (size_t)(-1);
			}
        }
        else {
            wcsLen = mbstowcs(NULL, mbstr, 0);
			if (*mbstr && !wcsLen) { /* wcsLen == 0 */
				size_t n = strlen(mbstr) + 1;
				wchar_t *dest = malloc(n * sizeof(wchar_t));
				if (dest) {
					wcsLen = mbstowcs(dest, mbstr, n);
					free(dest);
				}
			}
        }
        if (wcsLen == (size_t)(-1)) {
            ret = OPENSOAP_INVALID_MB_SEQUENCE;
        }
        else {
            size_t retLen = wcsLen + 1;
            *wcs = XwcsAlloc(retLen);
            if (*wcs) {
                *sz = mbstowcs(*wcs, mbstr, retLen);
                if (*sz == (size_t)(-1)) {
                    XwcsFree(wcs);
                    ret = OPENSOAP_INVALID_MB_SEQUENCE;
                }
                else {
                    ret = OPENSOAP_NO_ERROR;
                }
            }
            else {
                ret = OPENSOAP_MEM_BADALLOC;
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#Xwcsdup(wcsStr, wcsLen)
    Duplicate wchar_t's string.
	
    :Parameters
      :[in]      const wchar_t *((|wcsStr|))
        Wide character string
      :[in, out] size_t *((|wcsLen|))
        duplicated length. if *wcsLen > wcslen(wcsStr) then full copy.
    :Return Value
      :type
        wchar_t *
        wchar_t string
=end
 */
static
wchar_t *
Xwcsdup(/* [in]      */ const wchar_t *wcsStr,
        /* [in, out] */ size_t *wcsLen) {
    wchar_t *ret = NULL;
    size_t dummy = 0;
    size_t wcsStr_len = wcslen(wcsStr);
    if (!wcsLen) {
        wcsLen = &dummy;
    }
    if (*wcsLen) {
        if (*wcsLen > wcsStr_len) {
            *wcsLen = wcsStr_len;
        }
        ret = XwcsAlloc(*wcsLen + 1);

        if (ret) {
            wcscpy(ret, wcsStr);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringFree(obj)
    Free OpenSOAPString.
	
    :Parameters
      :[in] OpenSOAPObjectPtr ((|obj|))
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPStringFree(/* [in] */ OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (obj) {
        OpenSOAPStringPtr str = (OpenSOAPStringPtr)obj;
        XwcsFree(&str->stringEntity);
        ret = OpenSOAPObjectReleaseMembers(obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
            free(str);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringCreate(str)
    長さ 0 の OpenSOAP 文字列の作成。
    
    :Parameters
       :[out] OpenSOAPStringPtr *((|str|))
         作成した OpenSOAP 文字列のポインタの格納場所。
    :Return Value
	   :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCreate(/* [out] */ OpenSOAPStringPtr *str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        ret = OPENSOAP_MEM_BADALLOC;
        *str = malloc(sizeof(OpenSOAPString));
        if (*str) {
            ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)*str,
                                           OpenSOAPStringFree);
            if (OPENSOAP_SUCCEEDED(ret)) {
                (*str)->stringEntity = NULL;
                (*str)->length = 0;
                ret = OpenSOAPStringClear(*str);
            }
            else {
                free(*str);
                *str = NULL;
            }
        }
    }

    return ret;
}
    
/*
=begin
--- function#OpenSOAPStringCreateWithMB(mb_str, str)
    マルチバイト文字列により初期化した OpenSOAP 文字列の作成。
    
    :Parameters
      :[in]  const char *((|mb_str|))
        設定したいマルチバイト文字列。
      :[out] OpenSOAPStringPtr *((|str|))
        作成した OpenSOAP 文字列のポインタの格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCreateWithMB(/* [in]  */ const char *mb_str,
                           /* [out] */ OpenSOAPStringPtr *str) {
    int ret = OpenSOAPStringCreate(str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringSetStringMB(*str, mb_str);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringCreateWithWC(wc_str, str)
    ワイドキャラクタ文字列により初期化した OpenSOAP 文字列の作成。
    
    :Parameters
      :[in]  const wchar_t *((|wc_str|))
        設定したいワイドキャラクタ文字列。
      :[out] OpenSOAPStringPtr *((|str|))
        作成した OpenSOAP 文字列のポインタの格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCreateWithWC(/* [in]  */ const wchar_t *wc_str,
                           /* [out] */ OpenSOAPStringPtr *str) {
    int ret = OpenSOAPStringCreate(str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringSetStringWC(*str, wc_str);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringCreateWithCharEncodingString(char_enc, char_enc_str, str)
    Character-encoding 指定文字列により初期化した OpenSOAP 文字列の作成。
    
    :Parameters
      :[in]  const char *((|char_enc|))
        指定 Character-encoding 。
      :[in]  OpenSOAPByteArrayPtr ((|char_enc_str|))
        設定したい文字列データ。
      :[out] OpenSOAPStringPtr *((|str|))
        作成した OpenSOAP 文字列のポインタの格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCreateWithCharEncodingString(/* [in]  */ const char *char_enc,
                                           /* [in]  */ OpenSOAPByteArrayPtr
										   char_enc_str,
                                           /* [out] */ OpenSOAPStringPtr
										   *str) {
    int ret = OpenSOAPStringCreate(str);
    
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringSetCharEncodingString(*str,
                                                  char_enc,
                                                  char_enc_str);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringCreateWithUTF8(utf8Str, str)
    Create OpenSOAPString with UTF-8 encoded string.
    
    :Parameters
      :[in]  const char *((|utf8Str|))
        UTF-8 encoded string.
      :[out] OpenSOAPStringPtr *((|str|))
        pointer of OpenSOAPStringPtr 
    :Return Value
      :int
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCreateWithUTF8(/* [in]  */ const char *utf8Str,
							 /* [out] */ OpenSOAPStringPtr *str) {
    int ret = OpenSOAPStringCreate(str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringSetStringUTF8(*str, utf8Str);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringRetain(str)
    参照の追加。
    
    :Parameters
      :[in] OpenSOAPStringPtr ((|str|))
        OpenSOAP 文字列
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringRetain(/* [in] */ OpenSOAPStringPtr str) {
    return OpenSOAPObjectRetain((OpenSOAPObjectPtr)str);
}

/*
=begin
--- function#OpenSOAPStringRelease(str)
    参照の開放。参照先がなくなった場合、リソースの開放も行う。
    
    :Parameters
      :[in] OpenSOAPStringPtr ((|str|))
        OpenSOAP 文字列
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringRelease(/* [in] */ OpenSOAPStringPtr str) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)str);
}

/*
=begin
--- function#OpenSOAPStringGetLengthMB(str, len)
    現在の locale におけるマルチバイト文字列のサイズを取得する。
    
    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP 文字列
      :[out] size_t *((|len|))
        サイズを格納するバッファへのポインタ。

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetLengthMB(/* [in]  */ OpenSOAPStringPtr str,
                          /* [out] */ size_t *len) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && len) {
        *len = (str->length) ? 
            wcstombs(NULL, str->stringEntity, 0) : 0;
#if defined(MB_CUR_MAX)
		if (str->length && !*len) { /* wcstombs return 0 */
			size_t n = str->length * MB_CUR_MAX + 1;
			char *dest = malloc(n);
			if (dest) {
				*len = wcstombs(dest, str->stringEntity, n);
				free(dest);
			}
		}
#endif /* MB_CUR_MAX */		
        ret = (*len != (size_t)(-1)) ? OPENSOAP_NO_ERROR
            : OPENSOAP_INVALID_MB_SEQUENCE;
    }

    return ret;
}
        
/*
=begin
--- function#OpenSOAPStringGetLengthWC(str, len)
    ワイドキャラクタ文字列における長さを取得する。
    
    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP 文字列
      :[out] size_t *((|len|))
        長さを格納するバッファへのポインタ。

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetLengthWC(/* [in]  */ OpenSOAPStringPtr str,
                          /* [out] */ size_t *len) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && len) {
        *len = str->length;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}
        
/*
=begin
--- function#OpenSOAPStringGetStringMBAsByteArray(str, mb_str)
    現在の locale におけるマルチバイト文字列を取得する。
    
    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP 文字列
      :[out] OpenSOAPByteArrayPtr ((|mb_str|))
        文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPStringGetStringMBAsByteArray(/* [in]  */ OpenSOAPStringPtr str,
                                     /* [out] */ OpenSOAPByteArrayPtr mb_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (mb_str) {
        size_t  mb_str_len = 0;
        ret = OpenSOAPStringGetLengthMB(str, &mb_str_len);
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (!mb_str_len) {
                ret = OpenSOAPByteArrayClear(mb_str);
            }
            else {
                ret = OpenSOAPByteArrayResize(mb_str, mb_str_len + 1);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    unsigned char *mb_str_beg = NULL;
                    ret = OpenSOAPByteArrayBegin(mb_str, &mb_str_beg);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        mb_str_len = wcstombs(mb_str_beg,
                                              str->stringEntity,
                                              mb_str_len + 1);
                        ret = (mb_str_len == (size_t)(-1))
                            ? OPENSOAP_INVALID_MB_SEQUENCE :
                            OpenSOAPByteArrayResize(mb_str, mb_str_len);
                    }
                }
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringGetStringMBWithAllocator(str, memAllocator, len, mbStr)
    OpenSOAP String GetStringMB with memAllocator

    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP String Object pointer.
      :[in]  char * ( * ((|memAllocator|)) )(size_t)
        memAllocator function pointer. If NULL,
		then memAllocator like as  (char *)malloc(size).
      :[out] size_t * ((|len|))
	    length return buffer pointer. if NULL, then no effect.
      :[out] char ** ((|mbStr|))
	    MB string return buffer pointer. If NULL, then error.
		After this function call, You shuld release *mbStr's memory.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStringGetStringMBWithAllocator(/* [in]  */  OpenSOAPStringPtr str,
									   /* [in]  */  char *
									   (*memAllocator)(size_t),
									   /* [out] */ size_t *len,
									   /* [out] */ char **mbStr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (mbStr) {
        OpenSOAPByteArrayPtr mbStrBa = NULL;
		*mbStr = NULL;
        ret = OpenSOAPByteArrayCreate(&mbStrBa);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringGetStringMBAsByteArray(str, mbStrBa);
            if (OPENSOAP_SUCCEEDED(ret)) {
                const unsigned char *mbStrBeg = NULL;
                size_t lenDummy = 0;
				if (!len) {
					len = &lenDummy;
				}
                ret = OpenSOAPByteArrayGetBeginSizeConst(mbStrBa,
                                                         &mbStrBeg,
                                                         len);
                if (OPENSOAP_SUCCEEDED(ret)) {
					if (!memAllocator) {
						memAllocator = (char *(*)(size_t))malloc;
					}
					*mbStr = memAllocator(*len + 1);
                    if (*mbStr) {
                        memcpy(*mbStr, mbStrBeg, *len);
                        (*mbStr)[*len] = '\0';
                    }
                    else {
                        ret = OPENSOAP_MEM_BADALLOC;
                    }
                }
            }
            OpenSOAPByteArrayRelease(mbStrBa);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringGetStringWCWithAllocator(str, memAllocator, len, wcStr)
    OpenSOAP String GetStringWC with memAllocator

    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP String Object pointer.
      :[in]  void * ( * ((|memAllocator|)) )(size_t)
        memAllocator function pointer. If NULL,
		then memAllocator like as  (wchar_t *)malloc(size * sizeof(whar_t)).
      :[out] size_t * ((|len|))
	    length return buffer pointer. if NULL, then no effect.
      :[out] wchar_t ** ((|wcStr|))
	    WC string return buffer pointer. If NULL, then error.
		After this function call, You shuld release *wcStr's memory.
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  allocater size_t parameter's unit is byte, not wchar_t.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStringGetStringWCWithAllocator(/* [in]  */  OpenSOAPStringPtr str,
									   /* [in]  */  wchar_t *
									   (*memAllocator)(size_t),
									   /* [out] */ size_t *len,
									   /* [out] */ wchar_t **wcStr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (wcStr) {
		if (!memAllocator) {
			memAllocator = XwcsAlloc;
		}
		ret = OPENSOAP_MEM_BADALLOC;
		*wcStr = memAllocator(str->length);
		if (*wcStr) {
			if (len) {
				*len = str->length;
			}
			wcscpy(*wcStr, str->stringEntity);
			ret = OPENSOAP_NO_ERROR;
		}
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPStringGetStringUTF8WithAllocator(str, memAllocator, len, utf8Str)
    OpenSOAP String GetString as UTF-8 encoding with memAllocator

    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP String Object pointer.
      :[in]  char * ( * ((|memAllocator|)) )(size_t)
        memAllocator function pointer. If NULL,
		then memAllocator like as  (char *)malloc(size).
      :[out] size_t * ((|len|))
	    length return buffer pointer. if NULL, then no effect.
      :[out] char ** ((|utf8Str|))
	    UTF8 string return buffer pointer. If NULL, then error.
		After this function call, You shuld release *utf8Str's memory.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStringGetStringUTF8WithAllocator(/* [in]  */ OpenSOAPStringPtr str,
										 /* [in]  */ char *
										 (*memAllocator)(size_t),
										 /* [out] */ size_t *len,
										 /* [out] */ char **utf8Str) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && utf8Str) {
        OpenSOAPByteArrayPtr utf8StrBa = NULL;
		*utf8Str = NULL;
        ret = OpenSOAPByteArrayCreate(&utf8StrBa);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringGetCharEncodingString(str,
													  "UTF-8",
													  utf8StrBa);
            if (OPENSOAP_SUCCEEDED(ret)) {
                const unsigned char *utf8StrBeg = NULL;
                size_t lenDummy = 0;
				if (!len) {
					len = &lenDummy;
				}
                ret = OpenSOAPByteArrayGetBeginSizeConst(utf8StrBa,
                                                         &utf8StrBeg,
                                                         len);
                if (OPENSOAP_SUCCEEDED(ret)) {
					if (!memAllocator) {
						memAllocator = (char *(*)(size_t))malloc;
					}
					*utf8Str = memAllocator(*len + 1);
                    if (*utf8Str) {
                        memcpy(*utf8Str, utf8StrBeg, *len);
                        (*utf8Str)[*len] = '\0';
                    }
                    else {
                        ret = OPENSOAP_MEM_BADALLOC;
                    }
                }
            }
            OpenSOAPByteArrayRelease(utf8StrBa);
        }
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPStringGetStringMB(str, len, mb_str)
    現在の locale におけるマルチバイト文字列を取得する。
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
      :size_t * [in, out] ((|len|))
        サイズを格納するバッファへのポインタ。
        呼出前は ((|mb_str|)) のサイズを格納する。
      :char * [out] ((|mb_str|))
        文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetStringMB(OpenSOAPStringPtr /* [in] */ str,
                          size_t * /* [in, out] */ len,
                          char * /* [out] */ mb_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (len && mb_str) {
        OpenSOAPByteArrayPtr mb_str_ba = NULL;
        ret = OpenSOAPByteArrayCreate(&mb_str_ba);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringGetStringMBAsByteArray(str, mb_str_ba);
            if (OPENSOAP_SUCCEEDED(ret)) {
                const unsigned char *mb_str_beg = NULL;
                size_t mb_str_len = 0;
                ret = OpenSOAPByteArrayGetBeginSizeConst(mb_str_ba,
                                                         &mb_str_beg,
                                                         &mb_str_len);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (*len < mb_str_len) {
                        ret = OPENSOAP_PARAMETER_BADVALUE;
                    }
                    else {
                        memcpy(mb_str, mb_str_beg, mb_str_len);
                        mb_str[mb_str_len] = '\0';
                        *len = mb_str_len;
                        ret = OPENSOAP_NO_ERROR;
                    }
                }
            }
            OpenSOAPByteArrayRelease(mb_str_ba);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringGetStringWC(str, len, mb_str)
    ワイドキャラクタ文字列を取得する。
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
      :size_t * [in, out] ((|len|))
        サイズを格納するバッファへのポインタ。
        呼出前は ((|wc_str|)) の長さを格納する。
      :wchar_t * [out] ((|wc_str|))
        文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetStringWC(OpenSOAPStringPtr /* [in] */ str,
                          size_t * /* [in, out] */ len,
                          wchar_t * /* [out] */ wc_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && len && wc_str && (str->length < *len)) {
        wcscpy(wc_str, str->stringEntity);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
 */
static
int
OpenSOAPStringConvertCharEncodingWithSize(/* [in]  */ const char *from_enc,
                                          /* [in]  */ const unsigned char *
                                          from_beg,
                                          /* [in]  */ size_t from_sz,
                                          /* [in]  */ const char * to_enc,
                                          /* [out] */ OpenSOAPByteArrayPtr
                                          to_str);

/*
=begin
--- function#OpenSOAPStringGetCharEncodingString(str, charEnc, charEncStr)
    Get character encoding string.
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
      :const char * [in] ((|charEnc|))
        character encoding.
      :OpenSOAPByteArrayPtr [out] ((|charEncStr|))
        文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetCharEncodingString(/* [in]  */ OpenSOAPStringPtr str,
                                    /* [in]  */ const char *charEnc,
                                    /* [out] */
									OpenSOAPByteArrayPtr charEncStr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (charEncStr) {
		int isCurrentCodeSet = 0;
		ret = OpenSOAPLocaleIsCurrentCodeset(charEnc, &isCurrentCodeSet);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (isCurrentCodeSet) {
				ret = OpenSOAPStringGetStringMBAsByteArray(str,
														   charEncStr);
			}
			else {
				ret = OpenSOAPStringConvertCharEncodingWithSize("WCHAR_T",
																(const unsigned char *)(str->stringEntity),
																str->length
																* sizeof(wchar_t),
																charEnc,
																charEncStr);
				if (ret == OPENSOAP_ICONV_NOT_IMPL
					|| ret == OPENSOAP_UNKNOWN_CHARENCODE) {
					/* WCHAR_T encode unknown */
					OpenSOAPByteArrayPtr fromStr = NULL;
					/* */
					ret = OpenSOAPByteArrayCreate(&fromStr);
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = OpenSOAPStringGetStringMBAsByteArray(str, fromStr);
						if (OPENSOAP_SUCCEEDED(ret)) {
							ret = OpenSOAPStringConvertCharEncoding(NULL,
																	fromStr,
																	charEnc,
																	charEncStr);
						}
						/* */
						OpenSOAPByteArrayRelease(fromStr);
					}
				}
			}
		}
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringSetStringMB(str, mb_str)
    現在の locale におけるマルチバイト文字列を設定する。
    
    :Parameters
      :OpenSOAPStringPtr [in, out] ((|str|))
        OpenSOAP 文字列
      :char * [in] ((|mb_str|))
        設定したい文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringSetStringMB(OpenSOAPStringPtr /* [in, out] */ str,
                          const char * /* [in] */ mb_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        if (mb_str && *mb_str) {
            size_t  mbs_len = 0;
            wchar_t *str_ent = NULL;
            ret = Xmbstowcs(mb_str, &mbs_len, &str_ent);
            if (OPENSOAP_SUCCEEDED(ret)) {
                str->length = mbs_len;
                XwcsFree(&(str->stringEntity));
                str->stringEntity = str_ent;
            }
        }
        else {
            ret = OpenSOAPStringClear(str);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringSetStringWC(str, mb_str)
    ワイドキャラクタ文字列を設定する。
    
    :Parameters
      :OpenSOAPStringPtr [in, out] ((|str|))
        OpenSOAP 文字列
      :char * [in] ((|wc_str|))
        設定したい文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringSetStringWC(OpenSOAPStringPtr /* [in, out] */ str,
                          const wchar_t * /* [in] */ wc_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        if (wc_str && *wc_str) {
            size_t  wcs_len = -1;
            wchar_t *str_ent = Xwcsdup(wc_str, &wcs_len);
            if (str_ent) {
                str->length = wcs_len;
                XwcsFree(&(str->stringEntity));
                str->stringEntity = str_ent;
                ret = OPENSOAP_NO_ERROR;
            }
            else {
                ret = OPENSOAP_MEM_BADALLOC;
            }
        }
        else {
            ret = OpenSOAPStringClear(str);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringSetCharEncodingString(str, charEnc, charEncStr)
    現在の locale におけるマルチバイト文字列を設定する。
    
    :Parameters
      :OpenSOAPStringPtr [in, out] ((|str|))
        OpenSOAP 文字列
      :const char * [in] ((|charEnc|))
        設定したい文字列の Character Encoding.
      :OpenSOAPByteArrayPtr [in] ((|charEncStr|))
        設定したい文字列を格納するバッファへのポインタ。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringSetCharEncodingString(/* [in, out] */ OpenSOAPStringPtr str,
                                    /* [in] */ const char *charEnc,
                                    /* [in] */ OpenSOAPByteArrayPtr
									charEncStr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        OpenSOAPByteArrayPtr toStr = NULL;
        ret = OpenSOAPByteArrayCreate(&toStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringConvertCharEncoding(charEnc,
                                                    charEncStr,
                                                    "WCHAR_T",
                                                    toStr);
			if (ret == OPENSOAP_ICONV_NOT_IMPL
				|| ret == OPENSOAP_UNKNOWN_CHARENCODE) {
				/* WCHAR_T encode not found */
				ret = OpenSOAPStringConvertCharEncoding(charEnc,
														charEncStr,
														NULL,
														toStr);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* null terminated */
					ret = OpenSOAPByteArrayAppend(toStr, NULL, 1);
					if (OPENSOAP_SUCCEEDED(ret)) {
						const unsigned char *toStrBeg = NULL;
						ret = OpenSOAPByteArrayBeginConst(toStr, &toStrBeg);
						if (OPENSOAP_SUCCEEDED(ret)) {
							ret = OpenSOAPStringSetStringMB(str, toStrBeg);
						}
					}
				}
			}
			else if (OPENSOAP_SUCCEEDED(ret)) {
				/* null terminated */
				ret = OpenSOAPByteArrayAppend(toStr, NULL, sizeof(wchar_t));
				if (OPENSOAP_SUCCEEDED(ret)) {
					const unsigned char *toStrBeg = NULL;
					ret = OpenSOAPByteArrayBeginConst(toStr, &toStrBeg);
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = OpenSOAPStringSetStringWC(str,
														(const wchar_t *)
														toStrBeg);
					}
				}
			}

            OpenSOAPByteArrayRelease(toStr);
        }
    }

    return ret;
}


/*
=begin
--- function#OpenSOAPStringSetStringUTF8(str, utf8Str)
    Set UTF-8 encoded string.
    
    :Parameters
      :[out] OpenSOAPStringPtr ((|str|))
        OpenSOAP String
      :[in]  const char * ((|utf8Str|))
        UTF-8 encoded string.
    :Return Value
      :int
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringSetStringUTF8(/* [out] */ OpenSOAPStringPtr str,
							/* [in]  */ const char *utf8Str) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str) {
		if (!utf8Str || !*utf8Str) {
			/* if NULL or "" then clear */
			ret = OpenSOAPStringClear(str);
		}
		else {
			size_t utf8StrLen = strlen(utf8Str);
			OpenSOAPByteArrayPtr utf8StrBary = NULL;
			/* create UTF-8 string buffer */
			ret = OpenSOAPByteArrayCreateWithData(utf8Str,
												  utf8StrLen,
												  &utf8StrBary);
			if (OPENSOAP_SUCCEEDED(ret)) {
				/* set UTF-8 string */
				ret = OpenSOAPStringSetCharEncodingString(str,
														  "UTF-8",
														  utf8StrBary);

				/* release utf8 string buffer */
				OpenSOAPByteArrayRelease(utf8StrBary);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPStringClear(str)
    OpenSOAP 文字列をクリアして、長さを 0 にする。
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringClear(OpenSOAPStringPtr str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        wchar_t *cleared_str = XwcsRealloc(str->stringEntity, 1);

        if (cleared_str) {
            str->length = 0;
            cleared_str[0] = L'\0';
            str->stringEntity = cleared_str;
            ret = OPENSOAP_NO_ERROR;
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringAppendMB(str, mb_str, mb_len)
    現在の locale におけるマルチバイト文字列を OpenSOAP 文字列に
    追加する。
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
      :char * [in] ((|mb_str|))
        追加したい文字列を格納するバッファへのポインタ。
      :size_t [in] ((|mb_len|))
        追加したい文字数。0 の場合は、mb_str 全てを追加する。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringAppendMB(OpenSOAPStringPtr str,
                       const char *mb_str,
                       size_t       mb_len) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        if (mb_str && *mb_str) {
            size_t add_mbs_len = 0;
            if (add_mbs_len != (size_t)(-1)) {
                wchar_t *wc_str = NULL;
                ret = Xmbstowcs(mb_str, &mb_len, &wc_str);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPStringAppendWC(str, wc_str, mb_len);
                    XwcsFree(&wc_str);
                }
            }
            else {
                ret = OPENSOAP_INVALID_MB_SEQUENCE;
            }
        }
        else {
            ret = OPENSOAP_NO_ERROR;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringAppendWC(str, wc_str, wc_len)
    ワイドキャラクタ文字列を OpenSOAP 文字列に追加する。
    
    :Parameters
      :OpenSOAPStringPtr [in] ((|str|))
        OpenSOAP 文字列
      :char * [in] ((|wc_str|))
        追加したい文字列を格納するバッファへのポインタ。
      :size_t [in] ((|wc_len|))
        追加したい文字数。0 の場合は、mb_str 全てを追加する。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringAppendWC(OpenSOAPStringPtr str,
                       const wchar_t *wc_str,
                       size_t   wc_len) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str) {
        if (wc_str && *wc_str) {
            size_t new_len = str->length;
            wchar_t *new_str = NULL;
            size_t add_len = wcslen(wc_str);

            ret = OPENSOAP_NO_ERROR;
            if (!wc_len) {
                wc_len = add_len;
            }
            if (wc_len < add_len) {
                add_len = wc_len;
            }
            new_len += add_len;
            new_str = XwcsRealloc(str->stringEntity, new_len + 1);
            if (!new_str) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                wcsncat(new_str, wc_str, add_len);
                str->stringEntity = new_str;
                str->length = new_len;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringVFormatMB(str, format, ap)
    書式整形。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const char * [in] ((|format|))
            format 文字列。
        :va_list [in] ((|ap|))
            可変引数。
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringVFormatMB(/* [out] */ OpenSOAPStringPtr str,
                        /* [in]  */ const char *format,
                        /* [in]  */ va_list ap) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && format) {
#define BUFGRAWSIZE 256
        size_t  maxlen = strlen(format) + BUFGRAWSIZE;
        int len = 0;
        char *buf = NULL;
        char *old_buf = NULL;

        for (; buf = realloc(old_buf, (maxlen + 1));
             old_buf = buf) {
            len = vsnprintf(buf, maxlen, format, ap);
            if (len > -1) {
                break;
            }
            maxlen += BUFGRAWSIZE;
        }
        if (!buf) {
            free(old_buf);
            ret = OPENSOAP_MEM_BADALLOC;
        }
        else {
            ret = OpenSOAPStringSetStringMB(str, buf);
            free(buf);
        }
#undef BUFGRAWSIZE
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringVFormatWC(str, format, ap)
    書式整形。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const wchar_t * [in] ((|format|))
            format 文字列。
        :va_list [in] ((|ap|))
            可変引数。
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringVFormatWC(/* [out] */ OpenSOAPStringPtr str,
						/* [in]  */ const wchar_t *format,
						/* [in]  */ va_list ap) {
#if defined(HAVE_VSWPRINTF)    
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && format) {
#define BUFGRAWSIZE 256
        size_t  maxlen = wcslen(format) + BUFGRAWSIZE;
        int wcs_len = 0;
        wchar_t *wcs_buf = NULL;
        wchar_t *old_wcs_buf = NULL;

        for (; wcs_buf = XwcsRealloc(old_wcs_buf, (maxlen + 1));
             old_wcs_buf = wcs_buf) {
            wcs_len = vswprintf(wcs_buf, maxlen, format, ap);
            if (wcs_len > -1) {
                break;
            }
            maxlen += BUFGRAWSIZE;
        }
        if (!wcs_buf) {
            XwcsFree(&old_wcs_buf);
            ret = OPENSOAP_MEM_BADALLOC;
        }
        else {
            if (!wcs_len) {
                XwcsFree(&wcs_buf);
            }
            XwcsFree(&str->stringEntity);
            str->stringEntity = wcs_buf;
            str->length = wcs_len;

            ret = OPENSOAP_NO_ERROR;
        }
#undef BUFGRAWSIZE
    }
#else /* !HAVE_VSWPRINTF */
    OpenSOAPStringPtr formatStr = NULL;
    int ret = OpenSOAPStringCreateWithWC(format, &formatStr);
    if (OPENSOAP_SUCCEEDED(ret)) {
		char *formatMB = NULL;
		size_t formatMBSize = 0;
		ret = OpenSOAPStringGetStringMBWithAllocator(formatStr,
													 NULL,
													 &formatMBSize,
													 &formatMB);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPStringVFormatMB(str, formatMB, ap);
			
			/* */
			free(formatMB);
		}
    }
#endif /* HAVE_VSWPRINTF */

    return ret;
}

/*
=begin
--- function#OpenSOAPStringFormatMB(str, format, ...)
    書式整形。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const char * [in] ((|format|))
            format 文字列。
        :...
            可変引数。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFormatMB(/* [in] */ OpenSOAPStringPtr str,
                       /* [in] */ const char *format, ...) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    va_list ap;

    va_start(ap, format);
    ret = OpenSOAPStringVFormatMB(str, format, ap);
    va_end(ap);
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringFormatWC(str, format, ...)
    書式整形。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const wchar_t * [in] ((|format|))
            format 文字列。
        :...
            可変引数。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFormatWC(/* [in] */ OpenSOAPStringPtr str,
                       /* [in] */ const wchar_t *format, ...) {
    int ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
    va_list ap;

    va_start(ap, format);
    ret = OpenSOAPStringVFormatWC(str, format, ap);
    va_end(ap);
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringCompareMB(str, cmp_str, cmp_rslt)
    文字列比較。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const char * [in] ((|cmp_str|))
            比較文字列。
        :int * [out] ((|cmp_rslt|))
            比較結果。*cmp_rslt の値の意味は、strcmp と同等。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCompareMB(OpenSOAPStringPtr /* [in] */ str,
                        const char * /* [in] */ cmp_str,
                        int * /* [out] */ cmp_rslt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cmp_str) {
        wchar_t *cmp_wstr = NULL;
        ret = Xmbstowcs(cmp_str, NULL, &cmp_wstr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringCompareWC(str, cmp_wstr, cmp_rslt);
            XwcsFree(&cmp_wstr);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringCompareWC(str, cmp_str, cmp_rslt)
    文字列比較。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const wchar_t * [in] ((|cmp_str|))
            比較文字列。
        :int * [out] ((|cmp_rslt|))
            比較結果。*cmp_rslt の値の意味は、strcmp と同等。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCompareWC(OpenSOAPStringPtr /* [in] */ str,
                        const wchar_t * /* [in] */ cmp_str,
                        int * /* [out] */ cmp_rslt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && cmp_str && cmp_rslt) {
        *cmp_rslt = wcscmp(str->stringEntity, cmp_str);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringCompare(str, cmp_str, cmp_rslt)
    文字列比較。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :OpenSOAPStringPtr [in] ((|cmp_str|))
            比較文字列。
        :int * [out] ((|cmp_rslt|))
            比較結果。*cmp_rslt の値の意味は、strcmp と同等。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringCompare(OpenSOAPStringPtr /* [in] */ str,
                      OpenSOAPStringPtr /* [in] */ cmp_str,
                      int * /* [out] */ cmp_rslt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cmp_str) {
        ret = OpenSOAPStringCompareWC(str,
                                      cmp_str->stringEntity,
                                      cmp_rslt);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringFindStringMB(str, find_str, idx)
    文字列検索。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const char * [in] ((|find_str|))
            検索文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 検索結果インデックス。部分文字列が存在しなかった場合
            は値が (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFindStringMB(OpenSOAPStringPtr /* [in] */ str,
                           const char * /* [in] */ find_str,
                           size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (find_str) {
        wchar_t *find_wstr = NULL;
        ret = Xmbstowcs(find_str, NULL, &find_wstr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringFindStringWC(str, find_wstr, idx);
            XwcsFree(&find_wstr);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringFindStringWC(str, find_str, idx)
    文字列検索。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :const wchar_t * [in] ((|find_str|))
            検索文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 検索結果インデックス。部分文字列が存在しなかった場合
            は値が (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFindStringWC(OpenSOAPStringPtr /* [in] */ str,
                           const wchar_t * /* [in] */ find_str,
                           size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && find_str && idx &&
        (*idx != (size_t)(-1))) {
        size_t find_idx = *idx;
        *idx = (size_t)(-1);
        if  (find_idx < str->length) {
            const wchar_t *findstr_loc
                = wcsstr(str->stringEntity + find_idx, find_str);
            if (findstr_loc) {
                *idx = (findstr_loc - str->stringEntity);
            }
        }
        ret = OPENSOAP_NO_ERROR;
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringFindString(str, find_str, idx)
    文字列検索。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
            OpenSOAP String
        :OpenSOAPStringPtr [in] ((|find_str|))
            検索文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 検索結果インデックス。部分文字列が存在しなかった場合
            は値が (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFindString(OpenSOAPStringPtr /* [in] */ str,
                         OpenSOAPStringPtr /* [in] */ find_str,
                         size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (find_str) {
        ret = OpenSOAPStringFindStringWC(str,
                                         find_str->stringEntity,
                                         idx);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringFindIfStringIndex(str, find_func, find_func_opt, idx)
    文字列検索。

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
          OpenSOAP String
        :OpenSOAPStringFindIfFunc [in] ((|find_func|))
          検索条件関数。
        :void * [in] ((|find_func_opt|))
          検索条件関数第一引数。
        :size_t * [in, out] ((|idx|))
          [in]  検索開始インデックス。
          [out] 検索結果インデックス。部分文字列が存在しなかった場合
          は値が (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringFindIfStringIndex(OpenSOAPStringPtr /* [in] */ str,
                                OpenSOAPStringFindIfFunc /* [in] */ find_func,
                                void * /* [in] */ find_func_opt,
                                size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && find_func && idx &&
        (*idx != (size_t)(-1))) {
        size_t find_idx = *idx;
        *idx = (size_t)(-1);
        for (;find_idx < str->length; ++find_idx) {
            int judge = 0;
            ret = find_func(find_func_opt,
                            str->stringEntity[find_idx],
                            &judge);
            if (OPENSOAP_FAILED(ret) || judge) {
                break;
            }
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            *idx = find_idx;
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringReplaceStringMB(str, find_str, rplc_str, idx)
    文字列置換。

    :Parameters
        :OpenSOAPStringPtr [in, out] ((|str|))
            OpenSOAP String
        :const char * [in] ((|find_str|))
            検索文字列。
        :const char * [in] ((|rplc_str|))
            置換文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 置換開始インデックス。置換できなかった場合は値が
            (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringReplaceStringMB(OpenSOAPStringPtr /* [in, out] */ str,
                              const char * /* [in] */ find_str,
                              const char * /* [in] */ rplc_str,
                              size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (find_str && rplc_str) {
        wchar_t *find_wstr = NULL;
        ret = Xmbstowcs(find_str, NULL, &find_wstr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            wchar_t *rplc_wstr = NULL;
            ret = Xmbstowcs(rplc_str, NULL, &rplc_wstr);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStringReplaceStringWC(str,
                                                    find_wstr,
                                                    rplc_wstr,
                                                    idx);
                XwcsFree(&rplc_wstr);
            }
            XwcsFree(&find_wstr);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringReplaceStringWC(str, find_str, rplc_str, idx)
    文字列置換。

    :Parameters
        :OpenSOAPStringPtr [in, out] ((|str|))
            OpenSOAP String
        :const wchar_t * [in] ((|find_str|))
            検索文字列。
        :const wchar_t * [in] ((|rplc_str|))
            置換文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 置換開始インデックス。置換できなかった場合は値が
            (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringReplaceStringWC(OpenSOAPStringPtr /* [in, out] */ str,
                              const wchar_t * /* [in] */ find_str,
                              const wchar_t * /* [in] */ rplc_str,
                              size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (rplc_str) {
        ret = OpenSOAPStringFindStringWC(str, find_str, idx);
        if (OPENSOAP_SUCCEEDED(ret)) {
            size_t find_str_len = wcslen(find_str);
            size_t rplc_str_len = wcslen(rplc_str);
            size_t replaced_len = str->length + rplc_str_len - find_str_len;
            wchar_t *replaced_str = str->stringEntity;

            if (replaced_len != str->length) {
                if (replaced_len > str->length) {
                    replaced_str = XwcsRealloc(replaced_str,
                                               (replaced_len + 1));
                }
                if (replaced_str) {
                    wmemmove(replaced_str + *idx + rplc_str_len,
                             replaced_str + *idx + find_str_len,
                             str->length - *idx - find_str_len + 1);
                    
                    if (replaced_len < str->length) {
                        replaced_str = XwcsRealloc(replaced_str,
                                                   (replaced_len + 1));
                    }
                }
            }
            if (!replaced_str) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                wmemcpy(replaced_str + *idx,
                        rplc_str,
                        rplc_str_len);

                str->length = replaced_len;
                str->stringEntity = replaced_str;
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringReplaceString(str, find_str, rplc_str, idx)
    文字列置換。

    :Parameters
        :OpenSOAPStringPtr [in, out] ((|str|))
            OpenSOAP String
        :OpenSOAPStringPtr [in] ((|find_str|))
            検索文字列。
        :OpenSOAPStringPtr [in] ((|rplc_str|))
            置換文字列。
        :size_t * [in, out] ((|idx|))
            [in]  検索開始インデックス。
            [out] 置換開始インデックス。置換できなかった場合は値が
            (size_t)(-1) となる。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringReplaceString(OpenSOAPStringPtr /* [in, out] */ str,
                            OpenSOAPStringPtr /* [in] */ find_str,
                            OpenSOAPStringPtr /* [in] */ rplc_str,
                            size_t * /* [in, out] */ idx) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (find_str) {
        ret = OpenSOAPStringReplaceStringWC(str,
                                            find_str->stringEntity,
                                            rplc_str->stringEntity,
                                            idx);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringDuplicate(str, dup_str)
    Duplicate string.

    :Parameters
        :[in]  OpenSOAPStringPtr ((|str|))
            OpenSOAP String
        :[out] OpenSOAPStringPtr * ((|dup_str|))
            duplicate result.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringDuplicate(/* [in]  */ OpenSOAPStringPtr str,
                        /* [out] */ OpenSOAPStringPtr *dup_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && dup_str && (*dup_str != str)) {
        ret = (*dup_str) ?
            OpenSOAPStringSetStringWC(*dup_str,
                                      str->stringEntity)
            : OpenSOAPStringCreateWithWC(str->stringEntity,
                                         dup_str);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringGetSubstring(str, beg, len, sub_str)
    Duplicate string.

    :Parameters
        :OpenSOAPStringPtr [in] ((|str|))
          OpenSOAP String
        :size_t [in] ((|beg|))
          substring begin index.
        :size_t [in] ((|len|))
          substring length. if len == -1 then copy to end of string.
        :OpenSOAPStringPtr * [out] ((|sub_str|))
          substring
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetSubstring(OpenSOAPStringPtr /* [in] */ str,
                           size_t   /* [in] */ beg,
                           size_t   /* [in] */ len,
                           OpenSOAPStringPtr * /* [out] */ sub_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (str && sub_str && len && beg > str->length) {
        int is_create = !*sub_str;
        if (is_create) {
            ret = OpenSOAPStringCreate(sub_str);
        }
        if (!is_create || OPENSOAP_SUCCEEDED(ret)) {
            wchar_t *sub_beg = str->stringEntity + beg;
            size_t  sub_len = len;
            wchar_t *str_ent = Xwcsdup(sub_beg, &sub_len);
            if (str_ent) {
                (*sub_str)->length = sub_len;
                XwcsFree(&((*sub_str)->stringEntity));
                (*sub_str)->stringEntity = str_ent;
                ret = OPENSOAP_NO_ERROR;
            }
            else {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            if (is_create && OPENSOAP_FAILED(ret)) {
                OpenSOAPStringRelease(*sub_str);
            }
        }
    }

    return ret;
}

/*
  Unicode routines
*/

/*
=begin
= UTF-8
=end
*/

unsigned long
UTF8_USC_BOUNDARIES[] = {
	0x00000000L,
	0x00000080L,
	0x00000800L,
	0x00010000L,
	0x00200000L,
	0x04000000L,
	0x80000000L
};

#define UTF8_CUR_MAX \
((sizeof(UTF8_USC_BOUNDARIES) / sizeof(unsigned long)) - 1)

/*
  
 */
static
int
UTF8_FOLLOW_SIZE(unsigned long c) {
	int ret = 1;
	for (; ret != UTF8_CUR_MAX + 1 && c >= UTF8_USC_BOUNDARIES[ret];
		 ++ret) {
	}

	return ret - 1;
}

/*
=begin
--- function#OpenSOAPByteArrayAppendUCS4AsUTF8(toStr, ucs4)

	UCS4 -> UTF-8 convert and append

    :Parameters
        :[out] OpenSOAPByteArrayPtr ((|toStr|))
		   output buffer
        :[in]  unsigned long ((|ucs4|))
		  ucs4 value.
    :Return Value
      :int
	    error code.
=end
*/
static
int
OpenSOAPByteArrayAppendUCS4AsUTF8(/* [out] */ OpenSOAPByteArrayPtr toStr,
								  /* [in]  */ unsigned long ucs4) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	unsigned long c = ucs4;

	if (toStr) {
        unsigned char tmpBuf[UTF8_CUR_MAX];
		int addNum = UTF8_FOLLOW_SIZE(c);
		
		ret = OPENSOAP_INVALID_MB_SEQUENCE;
		/* this error code even not matched */
		if (addNum < UTF8_CUR_MAX) {
			long divNum = 1L;
			unsigned char maskVal = '\0';
			int appendSize = 0;
			/* */
			divNum <<= (6 * addNum);
			if (addNum) {
				maskVal = (~(0x7f >> addNum) & 0xff);
			}

			while (1) {
				unsigned long d_quot = c / divNum;
				unsigned long d_rem  = c % divNum;
					
				tmpBuf[appendSize++]
					= (unsigned char)d_quot | maskVal;
				if (!addNum) {
					break;
				}
				c = d_rem;
				--addNum;
				divNum >>= 6;
				maskVal = '\x80';
			}
			ret = OpenSOAPByteArrayAppend(toStr,
										  tmpBuf,
										  appendSize);
		}
	}

	return ret;
}

#if defined(HAVE_ICONV_H) && defined(ENABLE_SYSTEM_ICONV)
#include <iconv.h>

#include "LocaleImpl.h"

#if defined(__linux__)
/*
  Why second arg type of iconv declare in iconv.h is char ** at my environment?
 */
typedef char * ICONV_SECOND_ARG_TYPE_REF;
#else  /* !__linux__ */
typedef const char *ICONV_SECOND_ARG_TYPE_REF;
#endif /* !__linux__ */

/*
 */
static
int
OpenSOAPStringIConvOpen(/* [in]  */ const char *toEnc,
						/* [in]  */ const char *fromEnc,
						/* [out] */ iconv_t *cd) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (cd) {
		/* iconv_open */
		*cd = iconv_open(toEnc, fromEnc);
		if (*cd == (iconv_t)(-1)) {
			const char * const *toEncList = NULL;
			const char * const *fromEncList = NULL;
			const char * const *to = NULL;
			const char *toEncVal = NULL;
			int isFailedGetToEncList = 0;
			int isFailedGetFromEncList = 0;
			int isNotFirstToLoop = 0;

			/* get toEnc's codeset list */
			ret = OpenSOAPLocaleGetCodesetList(toEnc,
											   strlen(toEnc),
											   &toEncList);
			isFailedGetToEncList
				= OPENSOAP_FAILED(ret) || !toEncList || !*toEncList;
		
			/* get fromEnc's codeset list */
			ret = OpenSOAPLocaleGetCodesetList(fromEnc,
											   strlen(fromEnc),
											   &fromEncList);
			isFailedGetFromEncList 
				= OPENSOAP_FAILED(ret) || !fromEncList || !*fromEncList;
		
			to = toEncList;
			*cd = (iconv_t)(-1);
			for (toEncVal = toEnc;
				 *cd == (iconv_t)(-1) && toEncVal;
				 toEncVal = *to) {
				const char * const *from = fromEncList;
				const char *fromEncVal = fromEnc;
				int isNotFirstFromLoop = 0;
				for (; *cd == (iconv_t)(-1) && fromEncVal;
					 fromEncVal = *from) {
					/* iconv open */
					*cd = iconv_open(toEncVal, fromEncVal);
					/* fromEncList get fail check */
					if (isFailedGetFromEncList) {
						break;
					}
					/* if not first, increment list iterator */
					if (isNotFirstFromLoop) {
						++from;
					}
					else {
						isNotFirstFromLoop = 1;
					}
				}
				/* toEncList get fail check */
				if (isFailedGetToEncList) {
					break;
				}
				/* if not first, increment list iterator */
				if (isNotFirstToLoop) {
					++to;
				}
				else {
					isNotFirstToLoop = 1;
				}
			}
		}
		ret = (*cd == (iconv_t)(-1))
			? OPENSOAP_ICONV_NOT_IMPL
			: OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPStringIConvAppendTo(cd, fromBeg, fromSz, toStr)

=end
 */
static
int
OpenSOAPStringIConvAppendTo(/* [in]  */ iconv_t cd,
							/* [in]  */ const unsigned char *fromBeg,
							/* [in]  */ size_t fromSz,
							/* [out] */ OpenSOAPByteArrayPtr toStr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (toStr && cd != (iconv_t)(-1)) {
		ICONV_SECOND_ARG_TYPE_REF fromBuf
			= (ICONV_SECOND_ARG_TYPE_REF)fromBeg;

		ret = OPENSOAP_NO_ERROR;
		while (fromSz && OPENSOAP_SUCCEEDED(ret)) {
#define TMP_BUF_SZ  256
			char    tmpBuf[TMP_BUF_SZ];
			char    *toBuf = tmpBuf;
			size_t  toLeft = TMP_BUF_SZ;
			size_t  iconvResult = (size_t)(-1);

			while ((size_t)(-1) !=
				   (iconvResult = iconv(cd,
										&fromBuf, &fromSz,
										&toBuf, &toLeft))
				   && fromSz) {
			}

			if (iconvResult == (size_t)(-1)) {
				if (errno == EILSEQ) {
					ret = OPENSOAP_INVALID_MB_SEQUENCE;
					break;
				}
				else if (errno == EINVAL) {
					ret = OPENSOAP_INVALID_MB_SEQUENCE;
					break;
				}
			}
			ret = OpenSOAPByteArrayAppend(toStr,
										  tmpBuf,
										  toBuf - tmpBuf);
#define TMP_BUF_SZ  256
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPStringConvertCharEncodingWithSize(fromEnc, fromStr, toEnc, toStr)
    static function.
    Convert character encoding.
    This function isn't include OpenSOAPString member,
    but this has something to do with String processing.

    :Parameters
      :const char * [in] ((|fromEnc|))
        Convert source character encoding. If NULL or "" then current
        locale encoding.
      :const unsigned char * [in] ((|fromBeg|))
        Convert source string data.
      :const unsigned char * [in] ((|fromSz|))
        Convert source string data size.
      :const char * [in] ((|toEnc|))
        Convert distination character encoding. If NULL or "" then current
        locale encoding.
      :OpenSOAPByteArrayPtr [out] ((|toStr|))
        Convert distination string data. If fromEnc equal to toEnc,
        then copy fromStr to toStr.
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
 */
static
int
OpenSOAPStringConvertCharEncodingWithSize(/* [in]  */ const char *fromEnc,
                                          /* [in]  */ const unsigned char *
                                          fromBeg,
                                          /* [in]  */ size_t fromSz,
                                          /* [in]  */ const char * toEnc,
                                          /* [out] */ OpenSOAPByteArrayPtr
                                          toStr) {
    int ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
    if (!fromSz) {
        ret = OpenSOAPByteArrayClear(toStr);
    }
    else {
		/* current locale encoding */
        const char *curEnc = NULL;
		ret = OpenSOAPLocaleGetCurrentCodeset(&curEnc);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (!fromEnc || !(*fromEnc)) {
				fromEnc = curEnc;
			}
			if (!toEnc || !(*toEnc)) {
				toEnc = curEnc;
			}
    
			if (strcasecmp(fromEnc, toEnc) == 0) {
				/* duplicate */
				ret = OpenSOAPByteArrayClear(toStr);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPByteArrayAppend(toStr,
												  fromBeg,
												  fromSz);
				}
			}
			else {
				iconv_t cd = (iconv_t)(-1);
				ret = OpenSOAPStringIConvOpen(toEnc,
											  fromEnc,
											  &cd);
				if (OPENSOAP_SUCCEEDED(ret)) {

					/* clear toStr */
					ret = OpenSOAPByteArrayClear(toStr);
					if (OPENSOAP_SUCCEEDED(ret)) {
						/* iconv execute */
						ret = OpenSOAPStringIConvAppendTo(cd,
														  fromBeg,
														  fromSz,
														  toStr);
					}
					/* iconv close */
					iconv_close(cd);
				}
			}
		}
    }
    
    return ret;
}

#else  /* !HAVE_ICONV_H  && ENABLE_SYSTEM_ICONV */
/*
  Inchiki charencoding converter
*/
# if defined(NONGCC_WIN32)
/* for Win32 API's implement */
typedef wchar_t unichar_t;
# else /* !NONGCC_WIN32 */
/* for having no iconv no Win32 */
typedef unsigned short unichar_t;
# endif /* !NONGCC_WIN32 */

/* OpenSOAPUnicharArray */
typedef OpenSOAPByteArrayPtr OpenSOAPUnicharArrayPtr;

#define OpenSOAPUnicharArrayCreate(u_ary) OpenSOAPByteArrayCreate(u_ary)
#define OpenSOAPUnicharArrayRelease(u_ary) OpenSOAPByteArrayRelease(u_ary)
#define OpenSOAPUnicharArrayClear(u_ary) OpenSOAPByteArrayClear(u_ary)
#define OpenSOAPUnicharArrayResize(u_ary, sz) \
OpenSOAPByteArrayResize(u_ary, sz)
#define OpenSOAPUnicharArrayAppend(u_ary, data, sz) \
OpenSOAPByteArrayAppend(u_ary, \
(const unsigned char *)(data), (sz) * sizeof(unichar_t))
#define OpenSOAPUnicharArraySetData(u_ary, data, sz) \
OpenSOAPByteArraySetData(u_ary, \
(const unsigned char *)(data), (sz) * sizeof(unichar_t))
#define OpenSOAPUnicharArrayBegin(u_ary, beg) \
OpenSOAPByteArrayBegin(u_ary, (unsigned char **)(beg))

/*
=begin
--- function#OpenSOAPUnicharArrayGetBeginSizeConst(u_ary, beg, sz)
    Get begin const pointer and size of OpenSOAPUnicharArray.
	
    :Parameters
        :OpenSOAPUnicharArrayPtr [in] ((|u_ary|))
        :const unichar_t ** [out] ((|beg|))
        :size_t * [out] ((|sz|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
OpenSOAPUnicharArrayGetBeginSizeConst(OpenSOAPUnicharArrayPtr /* [in] */ u_ary,
                                      const unichar_t ** /* [out] */ beg,
                                      size_t * /* [out] */ sz) {
    int ret
        = OpenSOAPByteArrayGetBeginSizeConst(u_ary,
                                             (const unsigned char **)(beg),
                                             sz);
    if (OPENSOAP_SUCCEEDED(ret)) {
        *sz /= 2;
    }

    return ret;
}

typedef
int
(*FromUnicharFunc)(const unichar_t * /* [in] */ from_str,
                   size_t /* [in] */ from_len,
                   OpenSOAPByteArrayPtr /* [out] */ to_str);

typedef
int
(*ToUnicharFunc)(const unsigned char * /* [in] */ from_str,
                 size_t /* [in] */ from_sz,
                 OpenSOAPUnicharArrayPtr /* [out] */ to_str);

typedef
int
(*MBConvFunc)(const unsigned char * /* [in] */ from_str,
              size_t /* [in] */ from_sz,
              OpenSOAPByteArrayPtr /* [out] */ to_str);


/*---------------------------------------------------*/
#if 0
/* template */
/*
=begin
= Unicode
=end
*/

/*
=begin
--- function#UnicharTo( from_str, from_len, to_str)
    UnicharTo some encoding template.
	
    :Parameters
        :const unichar_t * [in] ((|from_str|))
        :size_t [in] ((|from_len|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharTo(const unichar_t * /* [in] */ from_str,
          size_t /* [in] */ from_len,
          OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
    }

    return ret;
}

/*
=begin
--- function#ToUnichar(from_str, from_sz, to_str)
    Some encoding ToUnichar template.
	
    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPUnicharArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
ToUnichar(const unsigned char * /* [in] */ from_str,
          size_t /* [in] */ from_sz,
          OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPUnicharArrayClear(to_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
    }

    return ret;
}

#endif
/*
=begin
= non Unicode(USASCII)
=end
*/

/*
=begin
--- function#USASCIIToUSASCII( from_str, from_sz, to_str)
    to US-ASCII if input not US-ASCII then error

    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
USASCIIToUSASCII(const unsigned char * /* [in] */from_str,
                 size_t /* [in] */ from_sz,
                 OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayResize(to_str, from_sz);
    
    if (OPENSOAP_SUCCEEDED(ret)) {
        unsigned char *to_i = NULL;
        ret = OpenSOAPByteArrayBegin(to_str, &to_i);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *c_end = from_str + from_sz;
            const unsigned char *c_i = from_str;
            for (; c_i != c_end && OPENSOAP_SUCCEEDED(ret);
                 ++c_i, ++to_i) {
                if (*c_i > 0x7f) {
                    ret = OPENSOAP_INVALID_MB_SEQUENCE;
                }
                else {
                    *to_i = *c_i;
                }
            }
        }
    }

    return ret;
}

/*
=begin
--- function#UnicharToUSASCII( from_str, from_len, to_str)
    unichar_t -> US-ASCII

    :Parameters
        :const unichar_t * [in] ((|from_str|))
        :size_t [in] ((|from_len|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToUSASCII(const unichar_t * /* [in] */ from_str,
                 size_t /* [in] */ from_len,
                 OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from_str && from_len) {
        /* clear */
        ret = OpenSOAPByteArrayClear(to_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            unsigned char *buf = malloc(from_len);
            if (!buf) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                const unichar_t *w_end = from_str + from_len;
                const unichar_t *w = from_str;
                unsigned char *to = buf;

                for (; w != w_end; *to++ = (unsigned char)*w++) {
                    if (*w > 0x7f) {
                        ret = OPENSOAP_INVALID_MB_SEQUENCE;
                        break;
                    }
                }

                if (w == w_end) {
                    ret = OpenSOAPByteArrayAppend(to_str,
                                                  buf,
                                                  from_len);
                }

                free(buf);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#USASCIIToUnichar( from_str, from_sz, to_str)
    US-ASCII to Unichar converter.
	
    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPUnicharArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
USASCIIToUnichar(const unsigned char * /* [in] */ from_str,
                 size_t /* [in] */ from_sz,
                 OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (from_str && from_sz) {
        /* clear */
        ret = OpenSOAPUnicharArrayClear(to_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *c_end = from_str + from_sz;
            const unsigned char *c = from_str;
            for (; c != c_end && OPENSOAP_SUCCEEDED(ret); ++c) {
                unichar_t u_c = *c;
                ret = OpenSOAPUnicharArrayAppend(to_str,
                                                 &u_c, 1);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#UnicharToLatin1(fromStr, fromLen, toStr)
    unichar_t -> ISO 8859-1

    :Parameters
        :[in]  const unichar_t * ((|from_str|))
        :[in]  size_t ((|from_len|))
        :[out] OpenSOAPByteArrayPtr ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToLatin1(/* [in]  */ const unichar_t *fromStr,
                /* [in]  */ size_t fromLen,
                /* [out] */ OpenSOAPByteArrayPtr toStr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromStr && fromLen) {
        /* clear */
        ret = OpenSOAPByteArrayClear(toStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            unsigned char *buf = malloc(fromLen);
            if (!buf) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                const unichar_t *wEnd = fromStr + fromLen;
                const unichar_t *w = fromStr;
                unsigned char *to = buf;

                for (; w != wEnd; *to++ = (unsigned char)*w++) {
                    if (*w > 0xff) {
                        ret = OPENSOAP_INVALID_MB_SEQUENCE;
                        break;
                    }
                }

                if (w == wEnd) {
                    ret = OpenSOAPByteArrayAppend(toStr,
                                                  buf,
                                                  fromLen);
                }

                free(buf);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#Latin1ToUnichar(fromStr, fromSz, toStr)
    Latin1 to Unichar converter.
	
    :Parameters
        :[in]  const unsigned char * ((|from_str|))
        :[in]  size_t ((|from_sz|))
        :[out] OpenSOAPUnicharArrayPtr ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
Latin1ToUnichar(/* [in]  */ const unsigned char *fromStr,
                /* [in]  */ size_t fromSz,
                /* [out] */ OpenSOAPUnicharArrayPtr toStr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (fromStr && fromSz) {
        /* clear */
        ret = OpenSOAPUnicharArrayClear(toStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *cEnd = fromStr + fromSz;
            const unsigned char *c = fromStr;
            for (; c != cEnd && OPENSOAP_SUCCEEDED(ret); ++c) {
                unichar_t uChar = *c;
                ret = OpenSOAPUnicharArrayAppend(toStr,
                                                 &uChar, 1);
            }
        }
    }

    return ret;
}

# if defined(NONGCC_WIN32)

#include <windows.h>

/*
=begin
= CP932 (Shift_JIS)
=end
*/
/*
=begin
--- function#UnicharToCP932( from_str, from_len, to_str)
    Unichar to Codepage 932 converter.
	
    :Parameters
        :const unichar_t * [in] ((|from_str|))
        :size_t [in] ((|from_len|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToCP932(const unichar_t * /* [in] */ from_str,
               size_t /* [in] */ from_len,
               OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (!from_len) {
        ret = OpenSOAPByteArrayClear(to_str);
    }
    else {
        const UINT encodingCP = 932;
        BOOL    is_used_def_char = FALSE;
        int mb_len
            = WideCharToMultiByte(encodingCP,
                                  WC_COMPOSITECHECK,
                                  from_str,
                                  from_len,
                                  NULL,
                                  0,
                                  NULL,
                                  &is_used_def_char);
    
        if (is_used_def_char) {
            ret = OPENSOAP_INVALID_MB_SEQUENCE;
        }
        else if (!mb_len) {
            DWORD last_error = GetLastError();
            ret = (last_error == ERROR_INSUFFICIENT_BUFFER)
                ? OPENSOAP_MEM_OUTOFRANGE
                : OPENSOAP_PARAMETER_BADVALUE;
        }
        else {
            ret = OpenSOAPByteArrayResize(to_str, (size_t)mb_len);
            if (OPENSOAP_SUCCEEDED(ret)) {
                unsigned char *to_str_beg = NULL;
                size_t to_str_sz = 0;
                ret = OpenSOAPByteArrayGetBeginSize(to_str,
                                                    &to_str_beg,
                                                    &to_str_sz);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    mb_len 
                        = WideCharToMultiByte(encodingCP,
                                              WC_COMPOSITECHECK,
                                              from_str,
                                              from_len,
                                              to_str_beg,
                                              to_str_sz,
                                              NULL,
                                              &is_used_def_char);
                        
                    if (is_used_def_char) {
                        ret = OPENSOAP_INVALID_MB_SEQUENCE;
                    }
                    else if (!mb_len) {
                        DWORD last_error = GetLastError();
                        ret = (last_error == ERROR_INSUFFICIENT_BUFFER)
                            ? OPENSOAP_MEM_OUTOFRANGE
                            : OPENSOAP_PARAMETER_BADVALUE;
                    }
                }
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#CP932ToUnichar( from_str, from_sz, to_str)
    Codepage 932 to unichar converter.
	
    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPUnicharArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
CP932ToUnichar(const unsigned char * /* [in] */ from_str,
               size_t /* [in] */ from_sz,
               OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (!from_sz) {
        ret = OpenSOAPUnicharArrayClear(to_str);
    }
    else {
        const UINT encodingCP = 932;
        int wc_len
            = MultiByteToWideChar(encodingCP,
                                  MB_ERR_INVALID_CHARS,
                                  from_str,
                                  from_sz,
                                  NULL,
                                  0);
        if (!wc_len) {
            DWORD last_error = GetLastError();
            ret = (last_error == ERROR_INSUFFICIENT_BUFFER)
                ? OPENSOAP_MEM_OUTOFRANGE
                : ((last_error == ERROR_NO_UNICODE_TRANSLATION)
                   ? OPENSOAP_INVALID_MB_SEQUENCE
                   : OPENSOAP_PARAMETER_BADVALUE);
        }
        else {
            unichar_t *wc_str = XwcsAlloc(wc_len + 1);
            if (!wc_str) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                wc_len
                    = MultiByteToWideChar(encodingCP,
                                          MB_ERR_INVALID_CHARS,
                                          from_str,
                                          from_sz,
                                          wc_str,
                                          wc_len);
                if (!wc_len) {
                    DWORD last_error = GetLastError();
                    ret = (last_error == ERROR_INSUFFICIENT_BUFFER)
                        ? OPENSOAP_MEM_OUTOFRANGE
                        : ((last_error == ERROR_NO_UNICODE_TRANSLATION)
                           ? OPENSOAP_INVALID_MB_SEQUENCE
                           : OPENSOAP_PARAMETER_BADVALUE);
                }
                else {
                    ret = OpenSOAPUnicharArraySetData(to_str,
                                                      wc_str,
                                                      wc_len);
                }

                XwcsFree(&wc_str);
            }
        }
    }
    
    return ret;
}

/*
  WCHAR_T
 */
static
int
UnicharToWCHAR_T(/* [in]  */ const unichar_t *from_str,
				 /* [in]  */ size_t from_len,
				 /* [out] */ OpenSOAPByteArrayPtr to_str) {
	int ret = OpenSOAPUnicharArraySetData(to_str, from_str, from_len);

	return ret;
}

static
int
WCHAR_TToUnichar(/* [in]  */ const unsigned char *from_str,
				 /* [in]  */ size_t from_sz,
				 /* [out] */ OpenSOAPUnicharArrayPtr to_str) {
	int ret = (from_sz % sizeof(wchar_t)) ? OPENSOAP_PARAMETER_BADVALUE
		: OpenSOAPByteArraySetData(to_str, from_str, from_sz);

	return ret;
}

# else /* !NONGCC_WIN32 */

static
int
UnicharToCP932(const unichar_t * /* [in] */ from_str,
               size_t /* [in] */ from_len,
               OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = UnicharToUSASCII(from_str,
                               from_len,
                               to_str);
        if (OPENSOAP_FAILED(ret)) {
            ret = OPENSOAP_ICONV_NOT_IMPL;
        }
    }

    return ret;
}

static
int
CP932ToUnichar(const unsigned char * /* [in] */ from_str,
               size_t /* [in] */ from_sz,
               OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPUnicharArrayClear(to_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = USASCIIToUnichar(from_str,
                               from_sz,
                               to_str);
        if (OPENSOAP_FAILED(ret)) {
            ret = OPENSOAP_ICONV_NOT_IMPL;
        }
    }

    return ret;
}

/*
  WCHAR_T
 */
static
int
UnicharToWCHAR_T(/* [in]  */ const unichar_t *from_str,
               /* [in]  */ size_t from_len,
               /* [out] */ OpenSOAPByteArrayPtr to_str) {
	int ret = OPENSOAP_ICONV_NOT_IMPL;

	return ret;
}

static
int
WCHAR_TToUnichar(/* [in]  */ const unsigned char *from_str,
               /* [in]  */ size_t from_sz,
               /* [out] */ OpenSOAPUnicharArrayPtr to_str) {
	int ret = OPENSOAP_ICONV_NOT_IMPL;

	return ret;
}

# endif /* !NONGCC_WIN32 */

#define IS_SJIS_FIRST(c) \
(0x81 <= (c) && (c) < 0xA0 || 0xE0 <= (c) && (c) < 0xFD)

#define IS_SJIS_SECOND(c) (0x40 <= (c) && (c) < 0xFD && (c) != 0x7F)

#define IS_SJIS_JISX0201KANA(c) (0xA0 <= (c) && (c) < 0xE0)

/*
=begin
= EUC-JP
=end
*/

/*
=begin
--- function#CP932ToEUCJP( from_str, from_sz, to_str)
    CP932(Shift_JIS) -> EUC-JP

    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPUnicharArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
CP932ToEUCJP(const unsigned char * /* [in] */ from_str,
             size_t /* [in] */ from_sz,
             OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        const unsigned char *c_end = from_str + from_sz;
        const unsigned char *c_i = from_str;
        while (c_i != c_end && OPENSOAP_SUCCEEDED(ret)) {
#define TMP_BUF_SZ 2
            unsigned char tmp_buf[TMP_BUF_SZ];
            unsigned char *tmp_i = tmp_buf;
            unsigned char c = *c_i++;
            if (IS_SJIS_FIRST(c)) {
                if (c_i == c_end) {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
                else if (!IS_SJIS_SECOND(*c_i)) {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
                else {
                    unsigned char s_c = *c_i++;
                    if (c < 0xF0) {
                        if (c < 0xA0) {
                            c -= '\x70';
                        }
                        else { /* c < '\xF0' */
                            c -= '\xB0';
                        }
                        if (s_c >= 0x80) {
                            --s_c;
                        }
                        c <<= 1;
                        if (s_c >= 0x9E) {
                            s_c -= '\x5E';
                        }
                        else {
                            --c;
                        }
                        s_c -= '\x1F';
                    }
                    *tmp_i++ = c + '\x80';
                    *tmp_i++ = s_c + '\x80';
                }
            }
            else {
                if (IS_SJIS_JISX0201KANA(c)) {
                    *tmp_i++ = '\x8E';
                    *tmp_i++ = c;
                }
                else if (c < 0x80) {
                    *tmp_i++ = c;
                }
                else {
                    ret = OPENSOAP_INVALID_MB_SEQUENCE;
                }
            }
            if (tmp_i != tmp_buf) {
                ret = OpenSOAPByteArrayAppend(to_str,
                                              tmp_buf,
                                              tmp_i - tmp_buf);
            }
#undef  TMP_BUF_SZ
        }
    }

    return ret;
}

/*
=begin
--- function#UnicharToEUCJP(from_str, from_len, to_str)
    Unichar to EUC-JP converter.
	
    :Parameters
        :const unichar_t * [in] ((|from_str|))
        :size_t [in] ((|from_len|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToEUCJP(const unichar_t * /* [in] */ from_str,
               size_t /* [in] */ from_len,
               OpenSOAPByteArrayPtr /* [out] */ to_str) {
    OpenSOAPByteArrayPtr mid_str = NULL;
    int ret = OpenSOAPByteArrayCreate(&mid_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = UnicharToCP932(from_str, from_len, mid_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            /* CP932 -> EUC-JP */
            const unsigned char *mid_begin = NULL;
            size_t mid_sz = 0;
            ret = OpenSOAPByteArrayGetBeginSizeConst(mid_str,
                                                     &mid_begin,
                                                     &mid_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = CP932ToEUCJP(mid_begin, mid_sz, to_str);
            }
        }

        OpenSOAPByteArrayRelease(mid_str);
    }

    return ret;
}


/*
=begin
--- function#UnicharToEUCJP( from_str, from_sz, to_str)
    EUC-JP -> CP932(Shift_JIS)

    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
#define IS_EUCJP_JISX0212(c) ((c) == 0x8F)
#define IS_EUCJP_JISX0201KANA(c) ((c) == 0x8E)

static
int
EUCJPToCP932(const unsigned char * /* [in] */ from_str,
             size_t /* [in] */ from_sz,
             OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        const unsigned char *c_end = from_str + from_sz;
        const unsigned char *c_i = from_str;
        while (c_i != c_end && OPENSOAP_SUCCEEDED(ret)) {
#define TMP_BUF_SZ 2
            unsigned char tmp_buf[TMP_BUF_SZ];
            unsigned char *tmp_i = tmp_buf;
            unsigned char c = *c_i++;
            if (IS_EUCJP_JISX0212(c)) {
                ret = OPENSOAP_INVALID_MB_SEQUENCE;
            }
            else if (IS_EUCJP_JISX0201KANA(c)) {
                if (c_i == c_end) {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
                else {
                    *tmp_i++ = *c_i++;
                }
            }
            else if (c < 0x80) { /* ASCII */
                *tmp_i++ = c;
            }
            else {
                if (c_i == c_end) {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
                else {
                    unsigned char p = *c_i++;
                    c &= 0x7f;
                    p &= 0x7f;
                    
                    c -= ' '; /* KU */
                    p += 0x1F; /* 63(0x3F) - ' '(0x20) */

                    if (c & 1) { /* KU odd */
                        ++c;
                        if (p >= 0x3F + 0x40) { /* ten >= 64 */
                            ++p;
                        }
                    }
                    else { /* KU even */
                        p += 0x5F; /* 158 - 63 */
                    }
                    c >>= 1;
                    c += 0x80L;
                    if (c > 0x9F) {
                        c += 0x40;
                    }

                    *tmp_i++ = c;
                    *tmp_i++ = p;
                }
            }
            if (tmp_i != tmp_buf) {
                ret = OpenSOAPByteArrayAppend(to_str,
                                              tmp_buf,
                                              tmp_i - tmp_buf);
            }
#undef  TMP_BUF_SZ
        }
    }

    return ret;
}

/*
=begin
--- function#EUCJPToUnichar( from_str, from_sz, to_str)
    EUC-JP -> Unichar

    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPUnicharArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
EUCJPToUnichar(const unsigned char * /* [in] */ from_str,
               size_t /* [in] */ from_sz,
               OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    OpenSOAPByteArrayPtr mid_str = NULL;
    int ret = OpenSOAPByteArrayCreate(&mid_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = EUCJPToCP932(from_str, from_sz, mid_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *mid_beg = NULL;
            size_t mid_sz = 0;
            ret = OpenSOAPByteArrayGetBeginSizeConst(mid_str,
                                                     &mid_beg,
                                                     &mid_sz);
            
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = CP932ToUnichar(mid_beg, mid_sz, to_str);
            }
        }

        OpenSOAPByteArrayRelease(mid_str);
    }

    return ret;
}

/*
=begin
= Unicode
=end
*/
/*
=begin
--- function#UCS2toUCS4( first, last, ucs4)
    UCS2    <-> UCS4
    unichar_t <-> unsigned long

    :Parameters
        :const unichar_t **[in,out] ((|first|))
        :const unichar_t * [in] ((|last|))
        :unsigned long * [out] ((|ucs4|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UCS2toUCS4(const unichar_t ** /* [in, out] */ first,
           const unichar_t * /* [in] */ last,
           unsigned long * /* [out] */ ucs4) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (first && *first && last && *first < last && ucs4) {
        const unichar_t *i = *first;
        unichar_t hc = *i++;
        ret = OPENSOAP_NO_ERROR;
        if (0xd800 <= hc && hc < 0xdc00) { /* High Surrogates */
            if (i < last) {
                unichar_t lc = *i++;
                if (0xdc00 <= lc && lc < 0xe000) { /* Low Surrogates */
                    *ucs4 = (hc - 0xd800) * 0x400 + (lc - 0xdc00) + 0x10000L;
                }
                else {
                    /* High Surrogates without Low Surrogates */
                    ret = OPENSOAP_INVALID_MB_SEQUENCE;
                }
            }
            else { /* not Low Surrogates */
                ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
            }
        }
        else if (0xdc00 <= hc && hc < 0xe000) { /* Low Surrogates without
                                                 * High Surrogates */
            ret = OPENSOAP_INVALID_MB_SEQUENCE;
        }
        else {                          /* not Surrogates */
            *ucs4 = hc;
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            *first = i;
        }
    }

    return ret;
}

/*
=begin
--- function#UCS4toUCS2( ucs4, first, last)
    UCS2    <-> UCS4
    unichar_t <-> unsigned long

    :Parameters
        :unsigned long * [in] ((|ucs4|))
        :const unichar_t **[in,out] ((|first|))
        :const unichar_t * [in] ((|last|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UCS4toUCS2(unsigned long /* [in] */ ucs4,
           unichar_t ** /* [in, out] */ first,
           unichar_t * /* [in] */ last) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (first && *first && last && *first < last) {
        unichar_t *i = *first;
        ret = OPENSOAP_NO_ERROR;
        if (ucs4 < 0x10000L) {
            *i++ = (unichar_t)ucs4;
        }
        else if (i + 1 < last) {
            const unsigned long div_num = 0x400L;
            unsigned long d_quot = ucs4 / div_num;
            unsigned long d_rem  = ucs4 % div_num;
			
            *i++ = (unichar_t)((d_quot - 0x40L) + 0xd800);
            *i++ = (unichar_t)(d_rem + 0xdc00);
        }
        else {
            ret = OPENSOAP_MEM_OUTOFRANGE;
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            *first = i;
        }
    }
    
    return ret;
}

#define IS_UTF8_FOLLOW(c) (((c) & 0xc0) == 0x80)
#define IS_UTF8_ASCII(c) (!((c) & 0x80))

static
int
UTF8_SIZE(unsigned char c) {
	int size = 0;

	if (c & 0x80) {
		unsigned char revCompVal = 0x3f;
		unsigned char revMaskVal = revCompVal >> 1;
		for (++size; size != UTF8_CUR_MAX
				 && (c & (~revMaskVal & 0xff)) != (~revCompVal & 0xff);
			 revCompVal = revMaskVal, revMaskVal >>= 1, ++size) {
		}
	}
	
	return (size != UTF8_CUR_MAX) ? (size + 1) : 0;
}

#define UNICODE_BOM (0xfeff)
#define UTF16_BOM_LE (0xfffe)
#define UTF32_BOM_LE (0xfffe0000L)

/*
=begin
--- function#UnicharToUTF8( fromStr, fromLen, toStr)

	Unichar -> UTF-8

    :Parameters
        :[in]  const unichar_t * ((|fromStr|))
        :[in]  size_t  ((|fromLen|))
        :[out] OpenSOAPByteArrayPtr ((|toStr|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToUTF8(/* [in]  */ const unichar_t *fromStr,
              /* [in]  */ size_t fromLen,
              /* [out] */ OpenSOAPByteArrayPtr toStr) {
    int ret = OpenSOAPByteArrayClear(toStr);
    if (OPENSOAP_SUCCEEDED(ret)) {
        const unichar_t *cItr = fromStr;
        const unichar_t *ed = fromStr + fromLen;

        while (cItr != ed && OPENSOAP_SUCCEEDED(ret)) {
            unsigned long c = 0;
            ret = UCS2toUCS4(&cItr, ed, &c);
            if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPByteArrayAppendUCS4AsUTF8(toStr,
														c);
			}
        }
    }
    
    return ret;
}

/*
=begin
--- function#UTF8ToUnichar( fromStr, fromSz, toStr)
	Unichar <- UTF-8

    :Parameters
        :const unsigned char *  [in]  ((|fromStr|))
        :size_t  [in]  ((|fromSz|))
        :OpenSOAPUnicharArrayPtr  [out]  ((|toStr|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UTF8ToUnichar(/* [in]  */ const unsigned char *fromStr,
              /* [in]  */ size_t fromSz,
              /* [out] */ OpenSOAPUnicharArrayPtr toStr) {
    int ret = OpenSOAPUnicharArrayClear(toStr);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OPENSOAP_PARAMETER_BADVALUE;
        if (fromStr && fromSz) {
            int isBomChecked = 0;
            const unsigned char *cItr = fromStr;
            const unsigned char *cEnd = fromStr + fromSz;

            ret = OPENSOAP_NO_ERROR;
            while (cItr != cEnd && OPENSOAP_SUCCEEDED(ret)) {
                unsigned char c = *cItr;
                size_t utf8Sz = UTF8_SIZE(c);
                unsigned long ucs4 = 0;
                if (!utf8Sz) {
                    ret = OPENSOAP_INVALID_MB_SEQUENCE;
                }
                else if (cItr + utf8Sz > cEnd) {
					/* incomplete */
					ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
				}
				else {
					const unsigned char *utf8End = cItr + utf8Sz;
					--utf8Sz;
					ucs4 = (c
							& (0xff >> (utf8Sz + (utf8Sz ? 2 : 1))));
					for (++cItr;
						 cItr != utf8End && IS_UTF8_FOLLOW(*cItr);
						 ++cItr) {
						ucs4 <<= 6;
						ucs4 += (0x3f & *cItr);
					}
					if (cItr != utf8End
						|| ucs4 <  UTF8_USC_BOUNDARIES[utf8Sz]
						|| ucs4 >= UTF8_USC_BOUNDARIES[utf8Sz + 1]) {
						ret = OPENSOAP_INVALID_MB_SEQUENCE;
					}
                }
                
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (isBomChecked || ucs4 != UNICODE_BOM) {
#define TMP_STRBUF_SZ 2
                        unichar_t strbuf[TMP_STRBUF_SZ + 1];
                        unichar_t * const bufEnd = &strbuf[TMP_STRBUF_SZ];
                        unichar_t *bufItr = strbuf;

                        ret = UCS4toUCS2(ucs4, &bufItr, bufEnd);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            ret =
                                OpenSOAPUnicharArrayAppend(toStr,
                                                           strbuf,
                                                           (bufItr - strbuf));
                        }
#undef TMP_STRBUF_SZ
                    }
                    isBomChecked = 1;
                }
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#UTF8ToUSASCII( from_str, from_sz, to_str)
    to US-ASCII if input not US-ASCII then error

    :Parameters
        :const unsigned char * [in] ((|from_str|))
        :size_t [in] ((|from_sz|))
        :OpenSOAPByteArrayPtr [out] ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UTF8ToUSASCII(const unsigned char * /* [in] */from_str,
              size_t /* [in] */ from_sz,
              OpenSOAPByteArrayPtr /* [out] */ to_str) {
    /* BOM check */
    static const unsigned char UTF8_BOM[] = "\xef\xbb\xbf";
    static size_t UTF8_BOM_SIZE = sizeof(UTF8_BOM) - 1;
    if (from_str && from_sz > UTF8_BOM_SIZE
        && strncmp(from_str, UTF8_BOM, UTF8_BOM_SIZE) == 0) {
        from_str += UTF8_BOM_SIZE;
    }

    return USASCIIToUSASCII(from_str, from_sz, to_str);
}

/*
=begin
= UTF-16
=end
*/
/*
=begin
--- function#UnicharToUTF16( from_str, from_len, to_str)

	Unichar -> UTF-16

    :Parameters
        :const unichar_t *  [in]  ((|from_str|))
        :size_t  [in] ((|from_len|))
        :OpenSOAPByteArrayPtr  [out]  ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToUTF16(const unichar_t * /* [in] */ from_str,
              size_t /* [in] */ from_len,
              OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        unichar_t bom = UNICODE_BOM;
        ret = OpenSOAPByteArrayAppend(to_str,
                                      (unsigned char *)(&bom),
                                      sizeof(unichar_t));
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret =
                OpenSOAPByteArrayAppend(to_str,
                                        (unsigned char *)from_str,
                                        from_len * sizeof(unichar_t));
        }
    }
    
    return ret;
}

/*
=begin
--- function#UTF16ToUnichar( from_str, from_sz, to_str)
	Unichar <- UTF-16

    :Parameters
        :const unsigned char *  [in]  ((|from_str|))
        :size_t  [in]  ((|from_sz|))
        :OpenSOAPUnicharArrayPtr  [out]  ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UTF16ToUnichar(const unsigned char * /* [in] */ from_str,
              size_t /* [in] */ from_sz,
              OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPUnicharArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        if (from_str && from_sz) {
            int is_le = 0;                  /* Is UTF-16LE? */
            int chk_bom = 0;
            const unsigned char *i = from_str;
            const unsigned char *ed = from_str + from_sz;

            ret = OPENSOAP_NO_ERROR;
            while (i < ed && OPENSOAP_SUCCEEDED(ret)) {
                unichar_t hc = *i++;
                if (i < ed) {
                    unichar_t lc = *i++;
                    unichar_t c = (is_le) ? ((lc << 8) + hc)
                        :((hc << 8) + lc);
                    if (!chk_bom) {
                        chk_bom = 1;
                        is_le = (c == UTF16_BOM_LE);
                        if (c == UNICODE_BOM || is_le) {
                            continue;
                        }
                    }
                    ret = OpenSOAPUnicharArrayAppend(to_str, &c, 1);
                }
                else {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
            }
        }
        else {
            ret = OPENSOAP_PARAMETER_BADVALUE;
        }
    }
    
    return ret;
}

/*
=begin
= UTF-32
=end
*/
/*
=begin
--- function#UnicharToUTF32( from_str, from_len, to_str)

	Unichar -> UTF-32

    :Parameters
        :const unichar_t *  [in]  ((|from_str|))
        :size_t  [in] ((|from_len|))
        :OpenSOAPByteArrayPtr  [out]  ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UnicharToUTF32(const unichar_t * /* [in] */ from_str,
               size_t /* [in] */ from_len,
               OpenSOAPByteArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPByteArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        const unichar_t *i  = from_str;
        const unichar_t *ed = from_str + from_len;
        unsigned long ucs4 = UNICODE_BOM;

        ret = OpenSOAPByteArrayAppend(to_str,
                                      (unsigned char *)(&ucs4),
                                      sizeof(ucs4));
        while (i < ed && OPENSOAP_SUCCEEDED(ret)) {
            if (OPENSOAP_FAILED(ret)) {
                break;
            }
            ret = UCS2toUCS4(&i, ed, &ucs4);
            if (OPENSOAP_FAILED(ret)) {
                break;
            }
            ret = OpenSOAPByteArrayAppend(to_str,
                                          (unsigned char *)(&ucs4),
                                          sizeof(ucs4));
        }
    }
    
    return ret;
}

/*
=begin
--- function#UTF32ToUnichar( from_str, from_sz, to_str)
	Unichar <- UTF-32

    :Parameters
        :const unsigned char *  [in]  ((|from_str|))
        :size_t  [in]  ((|from_sz|))
        :OpenSOAPUnicharArrayPtr  [out]  ((|to_str|))
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
UTF32ToUnichar(const unsigned char * /* [in] */ from_str,
               size_t /* [in] */ from_sz,
               OpenSOAPUnicharArrayPtr /* [out] */ to_str) {
    int ret = OpenSOAPUnicharArrayClear(to_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        if (from_str && from_sz) {
            int is_le = 0;                  /* Is UTF-32LE? */
            int chk_bom = 0;
            const unsigned char *i = from_str;
            const unsigned char *ed = from_str + from_sz;

            ret = OPENSOAP_NO_ERROR;
            while (i < ed && OPENSOAP_SUCCEEDED(ret)) {
                const unsigned char *i_ed = i + 4;
                if (i_ed < ed + 1) {
                    unsigned long c = 0;
#define TMP_STRBUF_SZ 2
                    unichar_t strbuf[TMP_STRBUF_SZ + 1];
                    unichar_t * const buf_ed = &strbuf[TMP_STRBUF_SZ];
                    unichar_t *buf_i = strbuf;
                    
                    if (is_le) {        /* LE convert */
                        for (; i != i_ed; ++i) {
                            c <<= 8;
                            c += *i;
                        }
                    }
                    else {              /* BE convert */
                        unsigned long t = 0x1000000L;
                        for (; i != i_ed; ++i) {
                            c += t * *i;
                            t >>= 8;
                        }
                    }
                    if (!chk_bom) {
                        chk_bom = 1;
                        is_le = (c == UTF32_BOM_LE);
                        if (c == UNICODE_BOM || is_le) {
                            continue;
                        }
                    }

                    ret = UCS4toUCS2(c, &buf_i, buf_ed);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPUnicharArrayAppend(to_str,
                                                         strbuf,
                                                         (buf_i - strbuf));
                    }
#undef TMP_STRBUF_SZ
                }
                else {
                    ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
                }
            }
        }
        else {
            ret = OPENSOAP_PARAMETER_BADVALUE;
        }
    }
    
    return ret;
}

/*---------------------------------------------------*/


typedef struct {
    const char *type_name;
    FromUnicharFunc fromUnichar;
    ToUnicharFunc   toUnichar;
} ConverterMapItem;

typedef struct {
    const char *from_enc;
    const char *to_enc;
    MBConvFunc  mb_conv;
} MBConverterMapItem;

static
const ConverterMapItem
ConverterMap[] = {
    { "US-ASCII", UnicharToUSASCII, USASCIIToUnichar},
    { "ISO-8859-1", UnicharToLatin1, Latin1ToUnichar},
    { "Shift_JIS", UnicharToCP932, CP932ToUnichar},
    { "EUC-JP", UnicharToEUCJP, EUCJPToUnichar},
    { "UTF-8", UnicharToUTF8, UTF8ToUnichar},
    { "UTF-16", UnicharToUTF16, UTF16ToUnichar},
    { "UTF-32", UnicharToUTF32, UTF32ToUnichar},
#if 0	
    { "WCHAR_T", UnicharToWCHAR_T, WCHAR_TToUnichar},
#endif	
    { NULL, NULL, NULL}
};

static
MBConverterMapItem
MBConverterMap[] = {
    {"US-ASCII", "Shift_JIS", USASCIIToUSASCII},
    {"US-ASCII", "EUC-JP", USASCIIToUSASCII},
    {"US-ASCII", "UTF-8", USASCIIToUSASCII},
    {"Shift_JIS", "US-ASCII", USASCIIToUSASCII},
    {"Shift_JIS", "EUC-JP", CP932ToEUCJP},
    {"EUC-JP", "US-ASCII", USASCIIToUSASCII},
    {"EUC-JP", "Shift_JIS", EUCJPToCP932},
    {"UTF-8", "US-ASCII", UTF8ToUSASCII},
    {NULL, NULL, NULL}
};

/*
=begin
--- function#GetConverter(enc_type)
    Get encoding converter.
	
    :Parameters
        :const char *  [in]  ((|enc_type|))
    :Return Value
      :const ConverterMapItem *
=end
*/
static
const ConverterMapItem *
GetConverter(const char * /* [in] */ enc_type) {
    const ConverterMapItem *ret = ConverterMap;

    for (; ret->type_name
             && (strcasecmp(ret->type_name, enc_type) != 0); ++ret) {
    }

    if (!(ret->type_name)) {
        ret = NULL;
    }

    return ret;
}

/*
=begin
--- function#GetMBConverter(from_enc, to_enc)
    Get Multibyte character encoding converter.
	
    :Parameters
        :const char * [in]  ((|from_enc|))
        :const char * [in]  ((|to_enc|))
    :Return Value
      :MBConvFunc
	:Note
	  This Function Is Static Function
=end
*/
static
MBConvFunc
GetMBConverter(const char *from_enc,
               const char *to_enc) {
    MBConvFunc  ret = NULL;
    MBConverterMapItem *i = MBConverterMap;
    for (; i->from_enc && i->to_enc; ++i) {
        if (strcasecmp(i->from_enc, from_enc) == 0
            && strcasecmp(i->to_enc, to_enc) == 0) {
            ret = i->mb_conv;
            break;
        }
    }

    return ret;
}

/*-------------------------------------------------------------------------*/

/*
--- function#OpenSOAPStringConvertCharEncodingWithSize(from_enc, from_str, to_enc, to_str)
    static function.
    Convert character encoding.
    This function isn't include OpenSOAPString member,
    but this has something to do with String processing.

    :Parameters
      :const char * [in] ((|from_enc|))
        Convert source character encoding. If NULL or "" then current
        locale encoding.
      :const unsigned char * [in] ((|from_beg|))
        Convert source string data.
      :const unsigned char * [in] ((|from_sz|))
        Convert source string data size.
      :const char * [in] ((|to_enc|))
        Convert distination character encoding. If NULL or "" then current
        locale encoding.
      :OpenSOAPByteArrayPtr [out] ((|to_str|))
        Convert distination string data. If from_enc equal to to_enc,
        then copy from_str to to_str.
    :Return Value
      :型
            int
      error code.

=end
 */
static
int
OpenSOAPStringConvertCharEncodingWithSize(/* [in]  */ const char *from_enc,
                                          /* [in]  */ const unsigned char *
                                          from_beg,
                                          /* [in]  */ size_t from_sz,
                                          /* [in]  */ const char *to_enc,
                                          /* [out] */ OpenSOAPByteArrayPtr
                                          to_str) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;
    if (!from_sz) {
        ret = OpenSOAPByteArrayClear(to_str);
    }
    else {
        const char *def_enc = NULL;
		ret = OpenSOAPLocaleGetCurrentCodeset(&def_enc);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (!from_enc) {
				from_enc = def_enc;
			}
			if (!to_enc) {
				to_enc = def_enc;
			}
			if (strcasecmp(from_enc, to_enc) == 0) {
				/* duplicate */
				ret = OpenSOAPByteArrayClear(to_str);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPByteArrayAppend(to_str,
												  from_beg,
												  from_sz);
				}
			}
			else {
				MBConvFunc mb_conv = GetMBConverter(from_enc, to_enc);
				if (mb_conv) {
					ret = (mb_conv)(from_beg, from_sz, to_str);
				}
				else {
					const ConverterMapItem *from_conv
						= GetConverter(from_enc);
					const ConverterMapItem *to_conv
						= GetConverter(to_enc);
					if (!from_conv || !to_conv) {
						ret = OPENSOAP_UNKNOWN_CHARENCODE;
					}
					else {
						OpenSOAPUnicharArrayPtr mid_str = NULL;
						ret = OpenSOAPUnicharArrayCreate(&mid_str);
						if (OPENSOAP_FAILED(ret)) {
							ret = OPENSOAP_MEM_BADALLOC;
						}
						else {
							ret = (from_conv->toUnichar)(from_beg, from_sz, mid_str);
							if (OPENSOAP_SUCCEEDED(ret)) {
								const unichar_t *mid_beg = NULL;
								size_t mid_sz = 0;
								ret = OpenSOAPUnicharArrayGetBeginSizeConst(mid_str,
																			&mid_beg,
																			&mid_sz);
								if (OPENSOAP_SUCCEEDED(ret)) {
									/* clear to_str */
									ret = OpenSOAPByteArrayClear(to_str);
									if (OPENSOAP_SUCCEEDED(ret)) {
										ret = (to_conv->fromUnichar)(mid_beg,
																	 mid_sz,
																	 to_str);
									}
								}
							}

							OpenSOAPUnicharArrayRelease(mid_str);
						}
					}
				}
			}
		}
    }
    
    return ret;
}

#endif /* !HAVE_ICONV_H && ENABLE_SYSTEM_ICONV */

/*
=begin
--- function#OpenSOAPStringConvertCharEncoding(fromEnc, fromStr, toEnc, toStr)
    Convert character encoding.
    This function isn't include OpenSOAPString member,
    but this has something to do with String processing.

    :Parameters
      :const char * [in] ((|fromEnc|))
        Convert source character encoding. If NULL or "" then current
        locale encoding.
      :OpenSOAPByteArrayPtr [in] ((|fromStr|))
        Convert source string data.
      :const char * [in] ((|toEnc|))
        Convert distination character encoding. If NULL or "" then current
        locale encoding.
      :OpenSOAPByteArrayPtr [out] ((|toStr|))
        Convert distination string data. If fromEnc equal to toEnc,
        then copy fromStr to toStr.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStringConvertCharEncoding(/* [in]  */ const char *fromEnc,
                                  /* [in]  */ OpenSOAPByteArrayPtr fromStr,
                                  /* [in]  */ const char *toEnc,
                                  /* [out] */ OpenSOAPByteArrayPtr toStr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromStr && toStr) {
        const unsigned char *fromBeg = NULL;
        size_t  fromSize = 0;
        
        ret = OpenSOAPByteArrayGetBeginSizeConst(fromStr,
                                                 &fromBeg, &fromSize);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringConvertCharEncodingWithSize(fromEnc,
                                                            fromBeg,
                                                            fromSize,
                                                            toEnc,
                                                            toStr);
        }
    }

    return ret;
}

/*
  XML Spec
Char :=#x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
*/
#define IsXMLChar(c) \
(((c) == 0x09L) || ((c) == 0x0aL) || ((c) == 0x0dL) \
 || (0x00020L <= (c) && (c) < 0x00d800L) \
 || (0x0e000L <= (c) && (c) < 0x00fffeL) \
 || (0x10000L <= (c) && (c) < 0x110000L))

/*
=begin
--- function#OpenSOAPStringConvertXMLCharRefToUTF8(utf8Beg, utf8BAry)
    Convert XML's CharRef to UTF-8 encoding.
    This function doesn't use OpenSOAPString member,
    but this has something to do with String processing.

    :Parameters
      :[in]  const unsigned char * ((|utf8Beg|))
        Convert source UTF-8 encode string data begin pointer.
      :[out] OpenSOAPByteArrayPtr ((|toStr|))
        Convert distination string data. 
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStringConvertXMLCharRefToUTF8(/* [in]  */ const unsigned char *utf8Beg,
									  /* [out] */
									  OpenSOAPByteArrayPtr utf8BAry) {
#if 1
	static const char CHAR_REF_PREFIX[] = "&#";
	static const size_t CHAR_REF_PREFIX_SIZE = sizeof(CHAR_REF_PREFIX) - 1;
	static const char CHAR_REF_POSTFIX = ';';
	/* clear */
	int	ret = OpenSOAPByteArrayClear(utf8BAry);

	if (utf8Beg && *utf8Beg) {
		const unsigned char *i = utf8Beg;
		const unsigned char *e = i + strlen(i);
		while (OPENSOAP_SUCCEEDED(ret)) {
			/* search XML's CharRef */
			const unsigned char *n = strstr(i, CHAR_REF_PREFIX);
			unsigned long digitValue = 10L;
			int isHex = 0;
			unsigned long ucs4 = 0L;
			size_t appendSize = 0L;
			if (!n) {
				/* no more XML's CharRef */
				n = e;
			}
			appendSize = n - i;

			/* append data */
			if (appendSize) {
				ret = OpenSOAPByteArrayAppend(utf8BAry,
											  i,
											  appendSize);
			}
			/* error or end of string loop exit */
			if (OPENSOAP_FAILED(ret) || !*n) {
				break;
			}
			n += CHAR_REF_PREFIX_SIZE;

			if (*n == 'x') {
				/* hex */
				isHex = 1;
				digitValue = 0x10L;
				++n;
			}
			
			for (ucs4 = 0; *n && isxdigit(*n); ++n) {
				unsigned long l
					= isdigit(*n) ? (*n - '0')
					: (toupper(*n) - 'A' + 0x0aL);
				if (l >= digitValue) {
					break;
				}
				ucs4 *= digitValue;
				ucs4 += l;
			}
			/* check XML Char Ref postfix */
			if (*n != CHAR_REF_POSTFIX) {
				ret = OPENSOAP_INCOMPLETE_MB_SEQUENCE;
				break;
			}
			++n;
			if (!IsXMLChar(ucs4)) {
				ret = OPENSOAP_INVALID_MB_SEQUENCE;
				break;
			}

			ret = OpenSOAPByteArrayAppendUCS4AsUTF8(utf8BAry,
													ucs4);

			i = n;
		}
	}

	return ret;
#else
	int ret = OpenSOAPByteArraySetData(utf8BAry,
									   utf8Beg,
									   (utf8Beg && *utf8Beg) ? strlen(utf8Beg)
									   : 0);

	return ret;
#endif
}

/*
=begin
--- function#OpenSOAPStringGetStringUSASCII(str, charEncStr)
    Get US-ASCII string.
    
    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP String
      :[out] OpenSOAPByteArrayPtr ((|charEncStr|))
        output buffer.
    :Return Value
      :int
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringGetStringUSASCII(/* [in]  */ OpenSOAPStringPtr str,
							   /* [out] */ OpenSOAPByteArrayPtr charEncStr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (str && charEncStr) {
		size_t fromSz = str->length;
		ret = OpenSOAPByteArrayResize(charEncStr, fromSz);
		if (OPENSOAP_SUCCEEDED(ret)) {
			const wchar_t *fromItr = str->stringEntity;
			const wchar_t *fromEnd = fromItr + fromSz;
			unsigned char *toItr = NULL;
			ret = OpenSOAPByteArrayBegin(charEncStr, &toItr);
			for (; OPENSOAP_SUCCEEDED(ret) && fromItr != fromEnd;
				 ++toItr, ++fromItr) {
				if (*fromItr & ~0x7f) {
					ret = OPENSOAP_INVALID_MB_SEQUENCE;
				}
				else {
					*toItr = (unsigned char)*fromItr;
				}
			}
		}
	}
    

    return ret;
}

/*
=begin
--- function#OpenSOAPStringIterateProc(str, iterateProc, beforeProc, afterProc, opt)
    Iterate procedure.
    
    :Parameters
      :[in]  OpenSOAPStringPtr ((|str|))
        OpenSOAP String
      :[in]  int ( * ((|iterateProc|)) )(unsigned long, size_t, size_t, void *)
	    iterate procedure.
      :[in]  int ( * ((|beforeProc|)) )(size_t, void *)
	    Before iterate procedure. If NULL, then no effect
      :[in]  int ( * ((|afterProc|)) )(size_t, void *)
	    After iterate procedure. If NULL, then no effect
      :[in, out] void * ((|opt|))
        iterateProc, beforeProc, and afterProc's option parameter.
    :Return Value
      :int
	    error code.
	:Note
	  yet.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPStringIterateProc(/* [in]  */ OpenSOAPStringPtr str,
						  /* [in]  */ int (*iterateProc)(unsigned long c,
														 size_t idx,
														 size_t len,
														 void *opt),
						  /* [in]  */ int (*beforeProc)(size_t len,
														void *opt),
						  /* [in]  */ int (*afterProc)(size_t len,
													   void *opt),
						  /* [in, out] */ void *opt) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (str && iterateProc) {
		size_t idx = 0;
		if (beforeProc) {
			ret = beforeProc(str->length, opt);
		}
		for (; OPENSOAP_SUCCEEDED(ret) && idx != str->length; ++idx) {
			unsigned long c = (unsigned long)(str->stringEntity[idx]);
			ret = iterateProc(c, idx, str->length, opt);
							  
		}
		if (OPENSOAP_SUCCEEDED(ret) && afterProc) {
			ret = afterProc(str->length, opt);
		}
	}

    return ret;
}
