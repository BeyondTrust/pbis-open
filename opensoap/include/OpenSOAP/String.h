/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: String.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_String_H
#define OpenSOAP_String_H

#include <OpenSOAP/ByteArray.h>
#include <stdarg.h>

/**
 * @file OpenSOAP/String.h
 * @brief OpenSOAP API String Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPString OpenSOAPString
     * @brief OpenSOAPString Structure Type Definition
     */
    typedef struct tagOpenSOAPString OpenSOAPString;

    /**
     * @typedef OpenSOAPString    *OpenSOAPStringPtr
     * @brief OpenSOAPString Pointer Type Definition
     */
    typedef OpenSOAPString    *OpenSOAPStringPtr;


    /**
      * @fn int OpenSOAPStringCreate(OpenSOAPStringPtr *str)
      * @brief Create 0 length OpenSOAP Character String
      * @param
      *    str OpenSOAPStringPtr * [out] ((|str|)) OpenSOAP String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCreate(/* [out] */ OpenSOAPStringPtr *str);
    
    /**
      * @fn int OpenSOAPStringCreateWithMB(const char *mb_str, OpenSOAPStringPtr *str)
      * @brief Create OpenSOAP Character String Initialized With a MultiByte String
      * @param
      *    mb_str const char * [in] ((|mb_str|)) MultiByte Character String
      * @param
      *    str OpenSOAPStringPtr * [out] ((|str|)) Created OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCreateWithMB(/* [in]  */ const char *mb_str,
                               /* [out] */ OpenSOAPStringPtr *str);

    /**
      * @fn int OpenSOAPStringCreateWithWC(const wchar_t *wc_str, OpenSOAPStringPtr *str)
      * @brief Create OpenSOAP Character String Initialized With a WideCharacter String
      * @param
      *    wc_str const wchar_t * [in] ((|wc_str|)) Wide Character String
      * @param
      *    str OpenSOAPStringPtr * [out] ((|str|)) Created OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCreateWithWC(/* [in]  */ const wchar_t *wc_str,
                               /* [out] */ OpenSOAPStringPtr *str);

    /**
      * @fn int OpenSOAPStringCreateWithCharEncodingString(const char *char_enc, OpenSOAPByteArrayPtr char_enc_str, OpenSOAPStringPtr *str)
      * @brief Create OpenSOAP Character String Initialized With a Character-encoding Specified String
      * @param
      *    char_enc const char * [in] ((|char_enc|)) Character Encoding
      * @param
      *    char_enc_str OpenSOAPByteArrayPtr [in] ((|char_enc_str|)) Character String
      * @param
      *    str OpenSOAPStringPtr * [out] ((|str|)) Created OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCreateWithCharEncodingString(/* [in]  */ const char * char_enc,
                                               /* [in]  */ OpenSOAPByteArrayPtr char_enc_str,
                                               /* [out] */ OpenSOAPStringPtr *str);

    /**
      * @fn int OpenSOAPStringCreateWithUTF8(const char *utf8Str, OpenSOAPStringPtr *str)
      * @brief Create OpenSOAP Character String Initialized With a UTF-8 Encoded String
      * @param
      *    utf8Str const char * [in] ((|utf8Str|)) UTF-8 encoded string
      * @param
      *    str OpenSOAPStringPtr * [out] ((|str|)) Created OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCreateWithUTF8(/* [in]  */ const char *utf8Str,
				 /* [out] */ OpenSOAPStringPtr *str);

    /**
      * @fn int OpenSOAPStringRetain(OpenSOAPStringPtr str)
      * @brief Add a Reference to a Resource
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringRetain(/* [in] */ OpenSOAPStringPtr str);

    /**
      * @fn int OpenSOAPStringRelease(OpenSOAPStringPtr str)
      * @brief Remove a Reference. If the number of references is zero, release the resource.
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringRelease(/* [in] */ OpenSOAPStringPtr str);

    /**
      * @fn int OpenSOAPStringGetLengthMB(OpenSOAPStringPtr str, size_t *len)
      * @brief Get length of MultiByte Stringv for the current locale.
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @param
      *    len size_t * [out] ((|len|)) length
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetLengthMB(/* [in]  */ OpenSOAPStringPtr str,
                              /* [out] */ size_t *len);
        
    /**
      * @fn int OpenSOAPStringGetLengthWC(OpenSOAPStringPtr str, size_t *len)
      * @brief Get length of WideCharacter String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @param
      *    len size_t * [out] ((|len|)) length
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetLengthWC(/* [in]  */ OpenSOAPStringPtr str,
                              /* [out] */ size_t *len);

    /**
      * @fn int OpenSOAPStringGetStringMBWithAllocator(OpenSOAPStringPtr str, char * (*memAllocator)(size_t), size_t *len, char **mbStr)
      * @brief OpenSOAP String GetStringMB with memAllocator
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @param
      *    memAllocator() char * [in] ( * ((|memAllocator|)) )(size_t) memAllocator function pointer. If NULL, memAllocator acts like (char *)malloc(size).
      * @param
      *    len size_t * [out] ((|len|)) length return buffer pointer. If NULL, no effect.
      * @param
      *    mbStr char ** [out] ((|mbStr|)) MB string return buffer pointer. If NULL, then error.
      * @note
      *    After calling this function, the memory allocated to *mbStr should be released.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringMBWithAllocator(/* [in]  */ OpenSOAPStringPtr str,
					   /* [in]  */ char * (*memAllocator)(size_t),
					   /* [out] */ size_t *len,
					   /* [out] */ char **mbStr);

    /**
      * @fn int OpenSOAPStringGetStringWCWithAllocator(OpenSOAPStringPtr str, wchar_t * (*memAllocator)(size_t), size_t *len, wchar_t **wcStr)
      * @brief OpenSOAP String GetStringWC with memAllocator
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @param
      *    memAllocator() wchar_t * [in] ( * ((|memAllocator|)) )(size_t) memAllocator function pointer. If NULL, memAllocator acts like (char *)malloc(size).
      * @param
      *    len size_t * [out] ((|len|)) length return buffer pointer. If NULL, no effect.
      * @param
      *    wcStr wchar_t ** [out] ((|wcStr|)) WC string return buffer pointer. If NULL, then error.
      * @note
      *    After calling this function, the memory allocated to *wcStr should be released.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringWCWithAllocator(/* [in]  */ OpenSOAPStringPtr str,
					   /* [in]  */ wchar_t * (*memAllocator)(size_t),
					   /* [out] */ size_t *len,
					   /* [out] */ wchar_t **wcStr);

    /**
      * @fn int OpenSOAPStringGetStringUTF8WithAllocator(OpenSOAPStringPtr str, char * (*memAllocator)(size_t), size_t *len, char **utf8Str)
      * @brief OpenSOAP String GetString as UTF-8 encoding with memAllocator
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP Character String
      * @param
      *    memAllocator() char * [in] ( * ((|memAllocator|)) )(size_t) memAllocator function pointer. If NULL, memAllocator acts like (char *)malloc(size).
      * @param
      *    len size_t * [out] ((|len|)) length return buffer pointer. If NULL, no effect.
      * @param
      *    utf8Str char ** [out] ((|utf8Str|)) UTF8 string return buffer pointer. If NULL, then error.
      * @note
      *    After calling this function, the memory allocated to *utf8Str should be released.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringUTF8WithAllocator(/* [in]  */ OpenSOAPStringPtr str,
					     /* [in]  */ char * (*memAllocator)(size_t),
					     /* [out] */ size_t *len,
					     /* [out] */ char **utf8Str);

    /**
      * @fn int OpenSOAPStringGetStringMB(OpenSOAPStringPtr str, size_t *len, char *mb_Str)
      * @brief Get MultiByte String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    len size_t * [in, out] ((|len|)) size.
      * @param
      *    mb_Str char * [out] ((|mb_str|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringMB(/* [in]      */ OpenSOAPStringPtr str,
                              /* [in, out] */ size_t *len,
                              /* [out]     */ char * mb_Str);
        
    /**
      * @fn int OpenSOAPStringGetStringWC(OpenSOAPStringPtr str, size_t *len, wchar_t *wc_str)
      * @brief Get WideCharacter String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    len size_t * [in, out] ((|len|)) size.
      * @param
      *    wc_str char * [out] ((|wc_str|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringWC(/* [in]      */ OpenSOAPStringPtr str,
                              /* [in, out] */ size_t *len,
                              /* [out]     */ wchar_t *wc_str);

    /**
      * @fn int OpenSOAPStringGetCharEncodingString(OpenSOAPStringPtr str, const char *char_enc, OpenSOAPByteArrayPtr char_enc_str)
      * @brief Get character encoding string
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    char_enc const char * [in] ((|charEnc|)) character encoding
      * @param
      *    char_enc_str OpenSOAPByteArrayPtr [out] ((|charEncStr|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetCharEncodingString(/* [in]  */ OpenSOAPStringPtr str,
                                        /* [in]  */ const char *char_enc,
                                        /* [out] */ OpenSOAPByteArrayPtr char_enc_str);
    
    /**
      * @fn int OpenSOAPStringSetStringMB(OpenSOAPStringPtr str, const char *mb_str)
      * @brief Set MultiByte Character string for the current locale
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    mb_str char * [in] ((|mb_str|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringSetStringMB(/* [in] */ OpenSOAPStringPtr str,
                              /* [in] */ const char *mb_str);
        
    /**
      * @fn int OpenSOAPStringSetStringWC(OpenSOAPStringPtr str, const wchar_t *wc_str)
      * @brief Set WideCharacter string
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    wc_str const wchar_t * [in] ((|wc_str|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringSetStringWC(/* [in] */ OpenSOAPStringPtr str,
                              /* [in] */ const wchar_t *wc_str);

    /**
      * @fn int OpenSOAPStringSetCharEncodingString(OpenSOAPStringPtr str, const char *char_enc, OpenSOAPByteArrayPtr charEncStr)
      * @brief Set character encoding string for the current locale
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    char_enc const char * [in] ((|charEnc|)) Character encoding
      * @param
      *    charEncStr OpenSOAPByteArrayPtr [in] ((|charEncStr|)) Character String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringSetCharEncodingString(/* [out] */ OpenSOAPStringPtr str,
                                        /* [in]  */ const char *char_enc,
                                        /* [in]  */ OpenSOAPByteArrayPtr charEncStr);

    /**
      * @fn int OpenSOAPStringSetStringUTF8(OpenSOAPStringPtr str, const char *utf8Str)
      * @brief Set UTF-8 encoded string
      * @param
      *    str OpenSOAPStringPtr [out] ((|str|))
      * @param
      *    utf8Str const char * [in] ((|utf8Str|))
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringSetStringUTF8(/* [out] */ OpenSOAPStringPtr str,
				/* [in]  */ const char *utf8Str);

    /**
      * @fn int OpenSOAPStringFormatMB(OpenSOAPStringPtr str, const char *format, ...)
      * @brief Format MultiByte String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    format const char * [in] ((|format|)) Format String
      * @param
      *    ... Variable parameters
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringFormatMB(/* [out] */ OpenSOAPStringPtr str,
                           /* [in]  */ const char *format, ...);

    /**
      * @fn int OpenSOAPStringFormatWC(OpenSOAPStringPtr str, const wchar_t *format, ...)
      * @brief Format WideCharacter String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    format const wchar_t * [in] ((|format|)) Format String
      * @param
      *    ... Variable parameters
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringFormatWC(/* [out] */ OpenSOAPStringPtr str,
                           /* [in]  */ const wchar_t *format, ...);

    /**
      * @fn int OpenSOAPStringVFormatMB(OpenSOAPStringPtr str, const char *format, va_list ap)
      * @brief Format MultiByte String with va_list
      * @param
      *    str 		[out] OpenSOAP String
      * @param
      *    format	[in]  Format String
      * @param
      *    ap		[in]  Variable parameters
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringVFormatMB(/* [out] */ OpenSOAPStringPtr str,
							/* [in]  */ const char *format,
							/* [in]  */ va_list ap);

    /**
      * @fn int OpenSOAPStringVFormatWC(OpenSOAPStringPtr str, const wchar_t *format, va_list ap)
      * @brief Format WideCharacter String with va_list
      * @param
      *    str 		[out] OpenSOAP String
      * @param
      *    format	[in]  Format String
      * @param
      *    ap		[in]  Variable parameters
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringVFormatWC(/* [out] */ OpenSOAPStringPtr str,
							/* [in]  */ const wchar_t *format,
							/* [in]  */ va_list ap);

    /**
      * @fn int OpenSOAPStringCompareMB(OpenSOAPStringPtr str, const char *cmp_str, int *cmp_rslt)
      * @brief Compare MultiByte String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    cmp_str const char * [in] ((|cmp_str|)) Character String
      * @param
      *    cmp_rslt int * [out] ((|cmp_rslt|)) Comparison Result. Same as for strcmp().
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCompareMB(/* [in]  */ OpenSOAPStringPtr str,
                            /* [in]  */ const char *cmp_str,
                            /* [out] */ int *cmp_rslt);

    /**
      * @fn int OpenSOAPStringCompareWC(OpenSOAPStringPtr str, const wchar_t *cmp_str, int *cmp_rslt)
      * @brief Compare WideCharacter String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    cmp_str const wchar_t * [in] ((|cmp_str|)) Character String
      * @param
      *    cmp_rslt int * [out] ((|cmp_rslt|)) Comparison Result. Same as for strcmp().
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCompareWC(/* [in]  */ OpenSOAPStringPtr str,
                            /* [in]  */ const wchar_t *cmp_str,
                            /* [out] */ int *cmp_rslt);

    /**
      * @fn int OpenSOAPStringCompare(OpenSOAPStringPtr str, OpenSOAPStringPtr cmp_str, int *cmp_rslt)
      * @brief Compare String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    cmp_str OpenSOAPStringPtr [in] ((|cmp_str|))  Character String
      * @param
      *    cmp_rslt int * [out] ((|cmp_rslt|)) Comparison Result. Same as for strcmp().
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringCompare(/* [in]  */ OpenSOAPStringPtr str,
                          /* [in]  */ OpenSOAPStringPtr cmp_str,
                          /* [out] */ int *cmp_rslt);

    /**
      * @fn int OpenSOAPStringFindStringMB(OpenSOAPStringPtr str, const char *find_str, size_t *idx)
      * @brief Find String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    find_str const char * [in] ((|find_str|)) Search string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Search result index.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringFindStringMB(/* [in]      */ OpenSOAPStringPtr str,
                               /* [in]      */ const char *find_str,
                               /* [in, out] */ size_t *idx);

    /**
      * @fn int OpenSOAPStringFindStringWC(OpenSOAPStringPtr str, const wchar_t *find_str, size_t *idx)
      * @brief Find String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    find_str const wchar_t * [in] ((|find_str|)) Search string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Search result index.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringFindStringWC(/* [in]      */ OpenSOAPStringPtr str,
                               /* [in]      */ const wchar_t *find_str,
                               /* [in, out] */ size_t *idx);

    /**
      * @fn int OpenSOAPStringFindString(OpenSOAPStringPtr str, OpenSOAPStringPtr find_str, size_t *idx)
      * @brief Find String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    find_str OpenSOAPStringPtr [in] ((|find_str|)) Search string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Search result index.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringFindString(/* [in]      */ OpenSOAPStringPtr str,
                             /* [in]      */ OpenSOAPStringPtr find_str,
                             /* [in, out] */ size_t *idx);

    /**
      * @typedef int (*OpenSOAPStringFindIfFunc)(void *opt, wchar_t c, int *judge)
      * @brief Find String Function Pointer Type Definition
      * @param
      *    opt void * [in] Find Function Option 
      * @param
      *    c wchar_t [in] 
      * @param
      *    judge int * [out] Result
      * @return
      *    Error Code
      */
    typedef int
    (*OpenSOAPStringFindIfFunc)(/* [in, out] */ void *opt,
                                /* [in]      */ wchar_t c,
                                /* [out]     */ int *judge);
	
    /**
      * @fn int OpenSOAPStringFindIfStringIndex(OpenSOAPStringPtr str, OpenSOAPStringFindIfFunc find_func, void *find_func_opt, size_t *idx)
      * @brief Find String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    find_func OpenSOAPStringFindIfFunc [in] ((|find_func|)) Search condition function
      * @param
      *    find_func_opt void * [in] ((|find_func_opt|)) Search condition function first parameter
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Search result index.
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPStringFindIfStringIndex(/* [in]  */ OpenSOAPStringPtr str,
                                    /* [in]  */ OpenSOAPStringFindIfFunc find_func,
                                    /* [in]  */ void *find_func_opt,
                                    /* [in, out] */ size_t *idx);
    /**
      * @fn int OpenSOAPStringReplaceStringMB(OpenSOAPStringPtr str, const char *find_str, const char *rplc_str, size_t *idx)
      * @brief Replace String
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    find_str const char * [in] ((|find_str|)) Search string
      * @param
      *    rplc_str const char * [in] ((|rplc_str|)) Replacement string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Replacement start index. If can't replace, set (size_)(-1).
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringReplaceStringMB(/* [in, out] */ OpenSOAPStringPtr str,
                                  /* [in]      */ const char *find_str,
                                  /* [in]      */ const char *rplc_str,
                                  /* [in, out] */ size_t *idx);
    
    /**
      * @fn int OpenSOAPStringReplaceStringWC(OpenSOAPStringPtr str, const wchar_t *find_str, const wchar_t *rplc_str, size_t *idx)
      * @brief Replace String
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    find_str const wchar_t * [in] ((|find_str|)) Search string
      * @param
      *    rplc_str const wchar_t * [in] ((|rplc_str|)) Replacement string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Replacement start index. If can't replace, set (size_)(-1).
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringReplaceStringWC(/* [in, out] */ OpenSOAPStringPtr str,
                                  /* [in]      */ const wchar_t *find_str,
                                  /* [in]      */ const wchar_t *rplc_str,
                                  /* [in, out] */ size_t *idx);
    
    /**
      * @fn int OpenSOAPStringReplaceString(OpenSOAPStringPtr str, OpenSOAPStringPtr find_str, OpenSOAPStringPtr rplc_str, size_t *idx)
      * @brief Replace String
      * @param
      *    str OpenSOAPStringPtr [in, out] ((|str|)) OpenSOAP String
      * @param
      *    find_str OpenSOAPStringPtr [in] ((|find_str|)) Search string
      * @param
      *    rplc_str OpenSOAPStringPtr [in] ((|rplc_str|)) Replacement string
      * @param
      *    idx size_t * [in, out] ((|idx|)) [in]:Search start index; [out]:Replacement start index. If can't replace, set (size_)(-1).
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringReplaceString(/* [in, out] */ OpenSOAPStringPtr str,
                                /* [in]      */ OpenSOAPStringPtr find_str,
                                /* [in]      */ OpenSOAPStringPtr rplc_str,
                                /* [in, out] */ size_t *idx);
    
    /**
      * @fn int OpenSOAPStringClear(OpenSOAPStringPtr str)
      * @brief Clear OpenSOAPString, set length to 0
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringClear(/* [out] */ OpenSOAPStringPtr str);

    /**
      * @fn int OpenSOAPStringAppendMB(OpenSOAPStringPtr str, const char *mb_str, size_t mb_len)
      * @brief Concatenate MB String To OpenSOAPString for current locale
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    mb_str char * [in] ((|mb_str|)) Concatenate string
      * @param
      *    mb_len size_t [in] ((|mb_len|)) Concatenate string number of characters. If 0, concatenate the entire mb_str.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringAppendMB(/* [out] */ OpenSOAPStringPtr str,
                           /* [in]  */ const char *mb_str,
                           /* [in]  */ size_t mb_len);

    /**
      * @fn int OpenSOAPStringAppendWC(OpenSOAPStringPtr str, const wchar_t *wc_str, size_t wc_len)
      * @brief Concatenate WC String To OpenSOAPString
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    wc_str wchar_t * [in] ((|wc_str|)) Concatenate string
      * @param
      *    wc_len size_t [in] ((|wc_len|)) Concatenate string number of characters. If 0, concatenate the entire wc_str.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringAppendWC(/* [out] */ OpenSOAPStringPtr str,
                           /* [in]  */ const wchar_t *wc_str,
                           /* [in]  */ size_t wc_len);

    /**
      * @fn int OpenSOAPStringDuplicate(OpenSOAPStringPtr str, OpenSOAPStringPtr *dup_str)
      * @brief Duplicate String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    dup_str OpenSOAPStringPtr * [out] ((|dup_str|)) Duplicate string
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringDuplicate(/* [in]  */ OpenSOAPStringPtr str,
                            /* [out] */ OpenSOAPStringPtr *dup_str);

    /**
      * @fn int OpenSOAPStringGetSubstring(OpenSOAPStringPtr str, size_t beg, size_t len, OpenSOAPStringPtr *sub_str)
      * @brief Duplicate String
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    beg size_t [in] ((|beg|)) Substring start index
      * @param
      *    len size_t [in] ((|len|)) Substring length. If len == -1 then copy to end of string.
      * @param
      *    sub_str OpenSOAPStringPtr * [out] ((|sub_str|)) Substring
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetSubstring(/* [in]  */ OpenSOAPStringPtr str,
                               /* [in]  */ size_t   beg,
                               /* [in]  */ size_t   len,
                               /* [out] */ OpenSOAPStringPtr *sub_str);

    /**
      * @fn int OpenSOAPStringConvertCharEncoding(const char *from_enc, OpenSOAPByteArrayPtr from_str, const char *to_enc, OpenSOAPByteArrayPtr to_str)
      * @brief Convert character encoding. This function does not use the OpenSOAP member, but is used for string processing.
      * @param
      *    from_enc const char * [in] ((|fromEnc|)) Convert source character encoding. If NULL or "" then current locale encoding.
      * @param
      *    from_str OpenSOAPByteArrayPtr [in] ((|fromStr|)) Convert source string data.
      * @param
      *    to_enc const char * [in] ((|toEnc|)) Convert destination character encoding. If NULL or "" then current locale encoding.
      * @param
      *    to_str OpenSOAPByteArrayPtr [out] ((|toStr|)) Convert destination string data. If fromEnc equal to toEnc, then copy fromStr to toStr.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringConvertCharEncoding(/* [in]  */ const char *from_enc,
                                      /* [in]  */ OpenSOAPByteArrayPtr from_str,
                                      /* [in]  */ const char *to_enc,
                                      /* [out] */ OpenSOAPByteArrayPtr to_str);

    /**
      * @fn int OpenSOAPStringConvertXMLCharRefToUTF8(const unsigned char *utf8Beg, OpenSOAPByteArrayPtr utf8BAry)
      * @brief Convert XML's CharRef to UTF-8 encoding. This function does not use the OpenSOAP member, but is used for string processing.
      * @param
      *    utf8Beg const unsigned char * [in] ((|utf8Beg|)) Convert source UTF-8 encode string data begin pointer.
      * @param
      *    utf8BAry OpenSOAPByteArrayPtr [out] ((|toStr|)) Convert destination string data. 
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringConvertXMLCharRefToUTF8(/* [in]  */ const unsigned char *utf8Beg,
					  /* [out] */ OpenSOAPByteArrayPtr utf8BAry);
    /**
      * @fn int OpenSOAPStringGetStringUSASCII(OpenSOAPStringPtr str, OpenSOAPByteArrayPtr charEncStr)
      * @brief Get US-ASCII string
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    charEncStr OpenSOAPByteArrayPtr [out] ((|charEncStr|)) output buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringGetStringUSASCII(/* [in]  */ OpenSOAPStringPtr str,
				   /* [out] */ OpenSOAPByteArrayPtr charEncStr);

    /**
      * @fn int OpenSOAPStringIterateProc(OpenSOAPStringPtr str, int (*iterateProc)(unsigned long c, size_t idx, size_t len, void *opt), int (*beforeProc)(size_t len, void *opt), int (*afterProc)(size_t len, void *opt), void *opt)
      * @brief Iterate procedure
      * @param
      *    str OpenSOAPStringPtr [in] ((|str|)) OpenSOAP String
      * @param
      *    iterateProc() int [in] ( * ((|iterateProc|)) )(unsigned long, size_t, size_t, void *) iterate procedure
      * @param
      *    beforeProc() int [in] ( * ((|beforeProc|)) )(size_t, void *) Before iterate procedure. If NULL, then no effect
      * @param
      *    afterProc() int [in] ( * ((|afterProc|)) )(size_t, void *) After iterate procedure. If NULL, then no effect
      * @param
      *    opt void * [in, out] ((|opt|)) iterateProc, beforeProc, and afterProc's option parameters.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringIterateProc(/* [in]  */ OpenSOAPStringPtr str,
			      /* [in]  */ int (*iterateProc)(unsigned long c, size_t idx, size_t len, void *opt),
			      /* [in]  */ int (*beforeProc)(size_t len, void *opt),
			      /* [in]  */ int (*afterProc)(size_t len, void *opt),
			      /* [in, out] */ void *opt);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_String_H */
