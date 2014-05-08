/*-----------------------------------------------------------------------------
 * $RCSfile: Serializer.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "Serializer.h"
#include <string.h>

/*
=begin
= OpenSOAP Serializer class
=end
 */

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(bool)(*fromValue, to)
    OpenSOAP Serializer of Boolean Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      Boolean-type, handled as INT  0,1,True,False
=end
 */
static
const wchar_t *
SOAPTrues[] = {
    L"true", L"1", NULL
};

static
const wchar_t *
SOAPFalses[] = {
    L"false", L"0", NULL
};

int
OPENSOAP_SERIALIZER_NAME(boolean)(/* [in]  */ void *fromValue,
								  /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
 
    if (fromValue && to) {
        ret = OpenSOAPStringSetStringWC(to,
                                        (*(int *)fromValue)
                                        ? SOAPTrues[0]
                                        : SOAPFalses[0]);
    }

    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(bool)(from, toValue)
    OpenSOAP Deserializer of Boolean Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      Boolean-type, handled as INT  0,1,True,False
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(boolean)(/* [in]  */ OpenSOAPStringPtr from,
									/* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
        int cmpRslt = 0;
        const wchar_t **cmpStrs = SOAPTrues;

        for (; *cmpStrs; ++cmpStrs) {
            ret = OpenSOAPStringCompareWC(from,
                                          *cmpStrs,
                                          &cmpRslt);
            if (OPENSOAP_FAILED(ret) || cmpRslt == 0) {
                break;
            }
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (*cmpStrs) {
                *(int *)toValue = 1;
            }
            else {
                for (cmpStrs = SOAPFalses; *cmpStrs; ++cmpStrs) {
                    ret = OpenSOAPStringCompareWC(from,
                                                  *cmpStrs,
                                                  &cmpRslt);
                    if (OPENSOAP_FAILED(ret) || cmpRslt == 0) {
                        break;
                    }
                }
                if (OPENSOAP_SUCCEEDED(ret) && *cmpStrs) {
                    *(int *)toValue = 0;
                }
            }
        }
    }

    return ret;
}

/* Base64 encoding/decoding */

#include <string.h>
#include <ctype.h>

static
const unsigned char
base64Code[]
= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static
const unsigned char
base64PadChar = '=';

static
unsigned char
getBase64Code(unsigned char c) {
	unsigned char ret = (unsigned char)(-1);
	const unsigned char *search
		= strchr(base64Code, c);
	if (search) {
		ret = (search - base64Code);
	}

	return ret;
}

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(base64Binary)(fromValue, to)
    OpenSOAP Serializer of Bibary(Base64Binary) Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      Binary-type(Base64Binary)ByteArray
=end
 */
int
OPENSOAP_SERIALIZER_NAME(base64Binary)(/* [in]  */ void *fromValue,
									   /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (fromValue && to) {
		OpenSOAPByteArrayPtr fromBary = *(OpenSOAPByteArrayPtr *)fromValue;
		const unsigned char *fromValueBeg = NULL;
		size_t fromSz = 0;
		ret = OpenSOAPByteArrayGetBeginSizeConst(fromBary,
												 &fromValueBeg,
												 &fromSz);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPByteArrayPtr toStr = NULL;
			ret = OpenSOAPByteArrayCreate(&toStr);
			if (OPENSOAP_SUCCEEDED(ret)) {
#define	B64UNIT_SIZE 3
				size_t	i = 0;
				size_t	paddingSz
					= (fromSz + B64UNIT_SIZE - 1)
					/ B64UNIT_SIZE * B64UNIT_SIZE;
				while (OPENSOAP_SUCCEEDED(ret) && i != paddingSz) {
#define	B64APPEND_SIZE 4
					unsigned char append_buf[B64APPEND_SIZE];
					size_t next_i = i + B64UNIT_SIZE;
					size_t append_fromSz
						= (next_i == paddingSz)
						? (fromSz - i) : B64UNIT_SIZE;
					const unsigned char *from_i
						= fromValueBeg + i;
					unsigned char *a_i = append_buf;
					
					*a_i++ = base64Code[(from_i[0] & 0xfc) >> 2];
					*a_i++ = base64Code[((from_i[0] & 0x03) << 4)
										 | ((append_fromSz > 1)
											? ((from_i[1] & 0xf0) >> 4)
											: '\0')];
					*a_i++ = (append_fromSz > 1)
						? base64Code[((from_i[1] & 0x0f) << 2)
									  | ((append_fromSz > 2)
										 ? ((from_i[2] & 0xc0) >> 6)
										  : '\0')]
						: base64PadChar;
					*a_i++ = (append_fromSz > 2)
						? base64Code[(from_i[2] & 0x3f)] : base64PadChar;

					ret = OpenSOAPByteArrayAppend(toStr,
												  append_buf,
												  B64APPEND_SIZE);
					i = next_i;
#undef	B64APPEND_SIZE
				}

				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPStringSetCharEncodingString(to,
															  "US-ASCII",
															  toStr);
				}
				OpenSOAPByteArrayRelease(toStr);
#undef	B64UNIT_SIZE
			}
		}
	}

    return ret;
}

static
int
OpenSOAPDeserializerBase64(/* [in] */ OpenSOAPByteArrayPtr from,
						   /* [out] */ OpenSOAPByteArrayPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && to) {
		ret = OpenSOAPByteArrayClear(to);
		if (OPENSOAP_SUCCEEDED(ret)) {
			const unsigned char *fromValue = NULL;
			size_t	fromSz = 0;
			ret = OpenSOAPByteArrayGetBeginSizeConst(from,
													 &fromValue,
													 &fromSz);
			if (OPENSOAP_SUCCEEDED(ret)) {
#define	B64DECUNIT_SIZE 4
				unsigned char decunit[B64DECUNIT_SIZE];
				unsigned char *dItr = decunit;
				size_t i = 0;
				while (i != fromSz && OPENSOAP_SUCCEEDED(ret)) {
					unsigned char c = fromValue[i++];
					if (!isspace(c)) {
						unsigned char d = getBase64Code(c);
						if (d != (unsigned char)(-1)) {
							*dItr++ = d;
						}
						else {
							ret = OPENSOAP_PARAMETER_BADVALUE;
							break;
						}
					}
					if (dItr == &decunit[B64DECUNIT_SIZE]) {
#define	B64UNIT_SIZE 3
						unsigned char appendData[B64UNIT_SIZE];
						size_t appendSize = 0;
					
						appendData[appendSize++]
							= ((decunit[0] & 0x3f) << 2)
							| ((decunit[1] & 0x30) >> 4);
						if (decunit[2] < 0x40) {
							appendData[appendSize++]
								= ((decunit[1] & 0x0f) << 4)
								| ((decunit[2] & 0x3c) >> 2);
							if (decunit[3] < 0x40) {
								appendData[appendSize++]
									= ((decunit[2] & 0x03) << 6)
									| (decunit[3] & 0x3f);
							}
						}
					
						ret = OpenSOAPByteArrayAppend(to,
													  appendData,
													  appendSize);
						dItr = decunit;
#undef	B64UNIT_SIZE
					}
				}
#undef	B64DECUNIT_SIZE
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(base64Binary)(from, toValue)
    OpenSOAP Deserializer of Bibary(Base64Binary) Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Deserializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      Binary-type(Base64Binary)ByteArray
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(base64Binary)(/* [in]  */ OpenSOAPStringPtr from,
										 /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		OpenSOAPByteArrayPtr	fromBAry = NULL;
		ret = OpenSOAPByteArrayCreate(&fromBAry);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPStringGetStringUSASCII(from,
												 fromBAry);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPDeserializerBase64(fromBAry,
												 *(OpenSOAPByteArrayPtr *)
												 toValue);
			}

			OpenSOAPByteArrayRelease(fromBAry);
		}
    }

    return ret;
}

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(double)(fromValue, to)
    OpenSOAP Serializer of Double Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      64bit floating
=end
 */
int
OPENSOAP_SERIALIZER_NAME(double)(/* [in]  */ void *fromValue,
								 /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        double val = *(double *)fromValue;
        ret = OpenSOAPStringFormatWC(to,
                                     L"%f",
                                     val);
    }
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(double)(from, toValue)
    OpenSOAP Deserializer of Double Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      64bit floating
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(double)(/* [in]  */ OpenSOAPStringPtr from,
								   /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		char *buf = NULL;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 NULL, /* not use length */
													 &buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
			*(double *)toValue = atof(buf);
			free(buf);
        }
    }
    return ret;
}

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(int)(fromValue, to)
    OpenSOAP Serializer of Integer Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      32bit
=end
 */
int
OPENSOAP_SERIALIZER_NAME(int)(/* [in]  */ void *fromValue,
							  /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        long val = *(long *)fromValue;
        ret = OpenSOAPStringFormatWC(to,
                                     L"%ld",
                                     val);
    }
    
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(int)(from, toValue)
    OpenSOAP Deserializer of Integer Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Serializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      32bit
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(int)(/* [in]  */ OpenSOAPStringPtr from,
								/* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		char *buf = NULL;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 NULL, /* not use length */
													 &buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
			*(long *)toValue = atol(buf);
			free(buf);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(short)(fromValue, to)
    OpenSOAP Serializer of Short Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      16bit
=end
 */
int
OPENSOAP_SERIALIZER_NAME(short)(/* [in]  */ void *fromValue,
								/* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        short val = *(short *)fromValue;
        ret = OpenSOAPStringFormatWC(to,
                                     L"%d",
                                     val);
    }
    
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(short)(from, toValue)
    OpenSOAP Deserializer of Short Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      16bit
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(short)(/* [in]  */ OpenSOAPStringPtr from,
								  /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		char *buf = NULL;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 NULL, /* not use length */
													 &buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
			*(short *)toValue = (short)atoi(buf);
			free(buf);
        }
    }
    return ret;
}

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(string)(fromValue, to)
    OpenSOAP Serializer of String Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      String
=end
 */
int
OPENSOAP_SERIALIZER_NAME(string)(/* [in]  */ void *fromValue,
								 /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        OpenSOAPStringPtr fromStr = *(OpenSOAPStringPtr *)fromValue;
        ret = OpenSOAPStringDuplicate(fromStr, &to);
    }
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(string)(from, toValue)
    OpenSOAP Deserializer of String Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      String
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(string)(/* [in]  */ OpenSOAPStringPtr from,
								   /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
	OpenSOAPStringPtr *toStr = (OpenSOAPStringPtr *)toValue;

    if (from && toStr
#if defined(STRING_DEDESIRALIZER_CHECKED)
		&& *toStr
#endif		
		) {
        ret = OpenSOAPStringDuplicate(from, toStr);
    }
    return ret;
}

/*-------------------------------*/
/* float serializer/deserializer */
/*-------------------------------*/

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(float)(fromValue, to)
    OpenSOAP Serializer of float Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      32bit floating point number.
=end
 */
int
OPENSOAP_SERIALIZER_NAME(float)(/* [in]  */ void *fromValue,
								/* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        float val = *(float *)fromValue;
        ret = OpenSOAPStringFormatWC(to,
                                     L"%f",
                                     val);
    }
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(float)(from, toValue)
    OpenSOAP Deserializer of float Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      32bit floating point number.
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(float)(/* [in]  */ OpenSOAPStringPtr from,
								  /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		char *buf = NULL;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 NULL, /* not use length */
													 &buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
			*(float *)toValue = (float)atof(buf);
			free(buf);
        }
    }
    return ret;
}

/*------------------------------*/
/* byte serializer/deserializer */
/*------------------------------*/

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(byte)(fromValue, to)
    OpenSOAP Serializer of byte Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer. This type is required
		 signed char *.
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      byte is integer type and it's region is [-128, 127].
=end
 */
int
OPENSOAP_SERIALIZER_NAME(byte)(/* [in]  */ void *fromValue,
							   /* [out] */ OpenSOAPStringPtr to) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        int val = *(signed char *)fromValue;
        ret = OpenSOAPStringFormatWC(to,
                                     L"%d",
                                     val);
    }
    
    return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(byte)(from, toValue)
    OpenSOAP Deserializer of byte Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      byte is integer and it's region is [-128, 127].
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(byte)(/* [in]  */ OpenSOAPStringPtr from,
								 /* [out] */ void *toValue) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (from && toValue) {
		char *buf = NULL;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 NULL, /* not use length */
													 &buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
			*(signed char *)toValue = (signed char)atoi(buf);
			free(buf);
        }
    }
    return ret;
}

/*----------------------------------*/
/* dateTime serializer/deserializer */
/*----------------------------------*/

#include <time.h>

/*
=begin
--- function#OPENSOAP_SERIALIZER_NAME(dateTime)(fromValue, to)
    OpenSOAP Serializer of dateTime Type

    :Parameters
      :[in]  void * ((|fromValue|))
        Serializer Object Assign Pointer. This type is required
		 signed char *.
      :[out] OpenSOAPStringPtr ((|to|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
int
OPENSOAP_SERIALIZER_NAME(dateTime)(/* [in]  */ void *fromValue,
								   /* [out] */ OpenSOAPStringPtr to) {
	static const char XMLdateTimeFormat[] = "%Y-%m-%dT%H:%M:%SZ";
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (fromValue && to) {
        const struct tm *val = (const struct tm *)fromValue;
#define BUFDEFSIZE 22
#define BUFGRAWSIZE 4
        size_t  maxlen = BUFDEFSIZE;
        char *buf = NULL;
        char *oldBuf = NULL;

        for (; buf = realloc(oldBuf, (maxlen + 1));
             oldBuf = buf) {
            size_t len = strftime(buf, maxlen, XMLdateTimeFormat, val);
            if (len < maxlen && len > 0) {
                break;
            }
            maxlen += BUFGRAWSIZE;
        }
        if (!buf) {
            free(oldBuf);
            ret = OPENSOAP_MEM_BADALLOC;
        }
        else {
            ret = OpenSOAPStringSetStringMB(to, buf);
            free(buf);
        }
#undef BUFGRAWSIZE
#undef BUFDEFSIZE
    }
    
    return ret;
}

static
int
isTermChars(char chr,
			const char *termChars,
			size_t termCharsSize) {
	int ret = 0;

	if (termChars && termCharsSize) {
		const char *tc = termChars;
		const char * const tcEnd = termChars + termCharsSize;
		for (; tc != tcEnd; ++tc) {
			if (*tc == chr) {
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

static
int
getLongValueWithTermChars(/* [in, out]  */ const char **str,
						  /* [in]  */ const char *strEnd,
						  /* [in]  */ const char *termChars,
						  /* [in]  */ size_t termCharsSize,
						  /* [out] */ long *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && *str && *str < strEnd && termChars && termCharsSize && value) {
		const char *strI   = *str;
		ret = OPENSOAP_NO_ERROR;
		*value = 0;
		for (; strI != strEnd
				 && !isTermChars(*strI, termChars, termCharsSize); ++strI) {
			if (!isdigit(*strI)) {
				ret = OPENSOAP_PARAMETER_BADVALUE;
				break;
			}
			*value *= 10;
			*value += (*strI - '0');
		}
		*str = strI;
	}

	return ret;
}

static
int
OpenSOAPDeserializerMBStrToDate(/* [in, out] */ const char **str,
								/* [in]      */ const char *strEnd,
								/* [out]     */ struct tm *toValue) {
	static const char YEAR_MONTH_TERM_CHARS[] = "-";
	static size_t YEAR_MONTH_TERM_CHARS_SIZE
		= sizeof(YEAR_MONTH_TERM_CHARS);
	static const char DAY_TERM_CHARS[] = "T";
	static size_t DAY_TERM_CHARS_SIZE
		= sizeof(DAY_TERM_CHARS);
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && *str && *str < strEnd && toValue) {
		long year = 0;
		ret = getLongValueWithTermChars(str,
										strEnd,
										YEAR_MONTH_TERM_CHARS,
										YEAR_MONTH_TERM_CHARS_SIZE,
										&year);
		if (OPENSOAP_SUCCEEDED(ret)) {
			long month = 0;
			++(*str);
			ret = getLongValueWithTermChars(str,
											strEnd,
											YEAR_MONTH_TERM_CHARS,
											YEAR_MONTH_TERM_CHARS_SIZE,
											&month);
			if (OPENSOAP_SUCCEEDED(ret)) {
				long day = 0;
				++(*str);
				ret = getLongValueWithTermChars(str,
												strEnd,
												DAY_TERM_CHARS,
												DAY_TERM_CHARS_SIZE,
												&day);
				if (OPENSOAP_SUCCEEDED(ret)) {
					if (month <= 0 || month > 12
						|| day <= 0 || day > 31) {
						ret = OPENSOAP_PARAMETER_BADVALUE;
					}
					else {
						toValue->tm_year = (year - 1900);
						toValue->tm_mon  = month - 1;
						toValue->tm_mday = day;
					}
				}
			}
		}
	}

	return ret;
}

static
int
OpenSOAPDeserializerMBStrToHourMin(/* [in, out] */ const char **str,
								   /* [in]      */ const char *strEnd,
								   /* [out]     */ struct tm *toValue) {
	static const char HOUR_MIN_TERM_CHARS[]	= ":";
	static size_t HOUR_MIN_TERM_CHARS_SIZE	= sizeof(HOUR_MIN_TERM_CHARS);
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && *str && *str < strEnd && toValue) {
		long hour = 0;
		ret = getLongValueWithTermChars(str,
										strEnd,
										HOUR_MIN_TERM_CHARS,
										HOUR_MIN_TERM_CHARS_SIZE,
										&hour);
		if (OPENSOAP_SUCCEEDED(ret)) {
			long minutes = 0;
			++(*str);
			ret = getLongValueWithTermChars(str,
											strEnd,
											HOUR_MIN_TERM_CHARS,
											HOUR_MIN_TERM_CHARS_SIZE,
											&minutes);
			if (OPENSOAP_SUCCEEDED(ret)) {
				if (hour < 0 || hour > 23
					|| minutes < 0 || minutes > 59) {
					ret = OPENSOAP_PARAMETER_BADVALUE;
				}
				else {
					toValue->tm_hour = hour;
					toValue->tm_min  = minutes;
				}
			}
		}
	}

	return ret;
}

static const char SECOND_TERM_CHARS[]	= ".-Z+";
static const size_t SECOND_TERM_CHARS_SIZE	= sizeof(SECOND_TERM_CHARS);
static const char *PRE_TZ_TERM_CHARS	= SECOND_TERM_CHARS + 1;
static const size_t PRE_TZ_TERM_CHARS_SIZE	= sizeof(SECOND_TERM_CHARS) - 1;

static
int
OpenSOAPDeserializerMBStrToSec(/* [in, out] */ const char **str,
							   /* [in]      */ const char *strEnd,
							   /* [in. out] */ struct tm *toValue) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && *str && *str < strEnd && toValue) {
		long sec = 0;
		ret = getLongValueWithTermChars(str,
										strEnd,
										SECOND_TERM_CHARS,
										SECOND_TERM_CHARS_SIZE,
										&sec);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (sec <= 0 || sec > 61) {
				ret = OPENSOAP_PARAMETER_BADVALUE;
			}
			else {
				toValue->tm_sec  = sec;
				if (**str == SECOND_TERM_CHARS[0]) { /* is point */
					long dummy = 0;
					++(*str);
					ret = getLongValueWithTermChars(str,
													strEnd,
													PRE_TZ_TERM_CHARS,
													PRE_TZ_TERM_CHARS_SIZE,
													&dummy);
				}
			}
		}
	}

	return ret;
}

static
int
OpenSOAPDeserializerMBStrToModifyTZ(/* [in, out] */ const char **str,
									/* [in]      */ const char *strEnd,
									/* [in. out] */ struct tm *toValue) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str && *str && *str < strEnd && toValue) {
		/*
		  const char *strI = *str;
		  */
		const char *preTzChar = strchr(PRE_TZ_TERM_CHARS, **str);
		int sign = preTzChar ? -(preTzChar - PRE_TZ_TERM_CHARS - 1) : 0;
		ret = OPENSOAP_NO_ERROR;
		if (sign) {
			struct tm tz;
			++(*str);
			ret = OpenSOAPDeserializerMBStrToHourMin(str, strEnd, &tz);
			if (OPENSOAP_SUCCEEDED(ret)) {
				toValue->tm_hour += sign * tz.tm_hour;
				toValue->tm_min  += sign * tz.tm_min;
			}
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* adjust tm_wday and tm_yday, etc. */
			time_t mktimeRet = (time_t)(-1);
			mktimeRet = mktime(toValue);
			if (mktimeRet == (time_t)(-1)) {
				ret = OPENSOAP_PARAMETER_BADVALUE;
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OPENSOAP_DESERIALIZER_NAME(dateTime)(from, toValue)
    OpenSOAP Deserializer of dateTime Type

    :Parameters
      :[in]  OpenSOAPStringPtr ((|from|))
        Deserializer Object Assign Pointer
      :[out] void * ((|toValue|))
	    Result Value Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
int
OPENSOAP_DESERIALIZER_NAME(dateTime)(/* [in]  */ OpenSOAPStringPtr from,
									 /* [out] */ void *toValue) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (from && toValue) {
		char *buf = NULL;
		size_t len = 0;
		ret = OpenSOAPStringGetStringMBWithAllocator(from,
													 NULL, /* use malloc */
													 &len,
													 &buf);
		if (OPENSOAP_SUCCEEDED(ret)) {
			const char * const bufEnd = buf + len + 1;
			const char *bufI = buf;
			struct tm *value = toValue;

			ret = OpenSOAPDeserializerMBStrToDate(&bufI,
												  bufEnd,
												  value);
			if (OPENSOAP_SUCCEEDED(ret)) {
				++bufI;
				ret = OpenSOAPDeserializerMBStrToHourMin(&bufI,
														 bufEnd,
														 value);
				if (OPENSOAP_SUCCEEDED(ret)) {
					++bufI;
					ret = OpenSOAPDeserializerMBStrToSec(&bufI,
														 bufEnd,
														 value);
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = OpenSOAPDeserializerMBStrToModifyTZ(&bufI,
																  bufEnd,
																  value);
					}
				}
			}
			free(buf);
		}
	}
	
	return ret;
}

/*
=begin
= Serializer registry code.
=end
 */

#include <OpenSOAP/StringHash.h>

typedef struct {
    OpenSOAPSerializerFunc      serializer;
    OpenSOAPDeserializerFunc    deserializer;
} StaticOpenSOAPSerializerPair, *StaticOpenSOAPSerializerPairPtr;

static
OpenSOAPStringHashPtr   serializerTable = NULL;

/*
=begin
--- function#createSerializerPair(serializer, deserializer)
    OpenSOAP create serializer pair

    :Parameters
      :OpenSOAPSerializerFunc [in] ((|serializer|))
        Serializer Function Program Pointer
      :OpenSOAPDeserializerFunc [out] ((|deserializer|))
        Deserializer Function Program Pointer
    :Return Value
      :StaticOpenSOAPSerializerPairPtr
	:Note
	  This Function Is Static Function
=end
 */
static
StaticOpenSOAPSerializerPairPtr
createSerializerPair(/* [in] */ OpenSOAPSerializerFunc serializer,
					 /* [in] */ OpenSOAPDeserializerFunc deserializer) {
	StaticOpenSOAPSerializerPairPtr ret
		= malloc(sizeof(StaticOpenSOAPSerializerPair));
	if (ret) {
		ret->serializer = serializer;
		ret->deserializer = deserializer;
	}
   
	return ret;
}

/*
=begin
--- function#deleteSerializerPair(obj, opt)
    Delete OpenSOAP serializer pair.

    :Parameters
      :[in]  void *((|obj|))
        SerializerPair 
      :[in]  void *((|opt|))
        NULL
    :Return Value
      :int
	    return OPENSOAP_NO_ERROR
=end
 */
static
int
deleteSerializerPair(/* [in] */ void *obj,
					 /* [in] */ void *opt) {
	int ret = OPENSOAP_NO_ERROR;
	StaticOpenSOAPSerializerPairPtr p = (StaticOpenSOAPSerializerPairPtr)obj;

	free(p);

	return ret;
}

/*
=begin
--- function#OpenSOAPSerializerRegisterString(soapTypename, serializer, deserializer)
    Serializer and deserializer register function.

    :Parameters
      :[in] OpenSOAPStringPtr ((|soapTypename|))
        typename.
      :[in] OpenSOAPSerializerFunc ((|serializer|))
        serializer
      :[in] OpenSOAPDeserializerFunc ((|deserializer|))
        deserializer

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
OpenSOAPSerializerRegisterString(/* [in] */ OpenSOAPStringPtr soapTypename,
								 /* [in] */ OpenSOAPSerializerFunc serializer,
								 /* [in] */ OpenSOAPDeserializerFunc deserializer) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapTypename && serializer && deserializer) {
        if (!serializerTable) {
            ret = OpenSOAPStringHashCreate(&serializerTable);
            if (OPENSOAP_FAILED(ret)) {
                serializerTable = NULL;
            }
        }
        if (serializerTable) {
            StaticOpenSOAPSerializerPairPtr serializerPair
                = createSerializerPair(serializer,
                                       deserializer);
            if (!serializerPair) {
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                ret = OpenSOAPStringHashSetValue(serializerTable,
                                                 soapTypename,
                                                 serializerPair);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSerializerRegisterMB(soapTypename, serializer, deserializer)
    Serializer and deserializer register function.(MB)

    :Parameters
      :[in] const char *((|soapTypename|))
        typename.
      :[in] OpenSOAPSerializerFunc ((|serializer|))
        serializer
      :[in] OpenSOAPDeserializerFunc ((|deserializer|))
        deserializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSerializerRegisterMB(/* [in] */ const char *soapTypename,
                             /* [in] */ OpenSOAPSerializerFunc serializer,
                             /* [in] */ OpenSOAPDeserializerFunc deserializer) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapTypename && *soapTypename) {
        OpenSOAPStringPtr   tname = NULL;
        ret = OpenSOAPStringCreateWithMB(soapTypename, &tname);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPSerializerRegisterString(tname,
												   serializer,
												   deserializer);
            OpenSOAPStringRelease(tname);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSerializerRegisterWC(soapTypename, serializer, deserializer)
    Serializer and deserializer register function.(WC)

    :Parameters
      : [in] const wchar_t *((|soapTypename|))
        typename.
      :[in] OpenSOAPSerializerFunc ((|serializer|))
        serializer
      :[in] OpenSOAPDeserializerFunc ((|deserializer|))
        deserializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSerializerRegisterWC(/* [in] */ const wchar_t *soapTypename,
                             /* [in] */ OpenSOAPSerializerFunc serializer,
                             /* [in] */ OpenSOAPDeserializerFunc deserializer) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapTypename && *soapTypename) {
        OpenSOAPStringPtr   tname = NULL;
        ret = OpenSOAPStringCreateWithWC(soapTypename, &tname);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPSerializerRegisterString(tname,
												   serializer,
												   deserializer);
            OpenSOAPStringRelease(tname);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPGetSerializerPair(soapTypename, serializerPair)
    Get registered serializer.

    :Parameters
      :[in]  OpenSOAPStringPtr ((|soapTypename|))
        typename.
      :[out] StaticOpenSOAPSerializerPairPtr * ((|serializerPair|))
        serializer
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
 */
static
/* extern */
int
/* OPENSOAP_API */
OpenSOAPGetSerializerPair(/* [in] */ OpenSOAPStringPtr soapTypename,
                          /* [out] */ StaticOpenSOAPSerializerPairPtr *serializerPair) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapTypename && serializerPair) {
        ret = OpenSOAPStringHashGetValue(serializerTable,
                                         soapTypename,
                                         (void **)serializerPair);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPGetSerializer(soapTypename, serializer)
    Get registered serializer.(MB)

    :Parameters
      :[in]  OpenSOAPStringPtr ((|soapTypename|))
        typename.
      :[out] OpenSOAPSerializerFunc * ((|serializer|))
        serializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetSerializer(/* [in] */  OpenSOAPStringPtr soapTypename,
                      /* [out] */ OpenSOAPSerializerFunc *serializer) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (soapTypename && serializer) {
        StaticOpenSOAPSerializerPairPtr serializerPair = NULL;
        ret = OpenSOAPGetSerializerPair(soapTypename,
                                        &serializerPair);
        if (OPENSOAP_SUCCEEDED(ret)) {
            *serializer = serializerPair->serializer;
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPGetSerializerMB(soapTypename, serializer)
    Get registered serializer.(MB)

    :Parameters
      :[in]  const char * ((|soapTypename|))
        typename.
      :[out] OpenSOAPSerializerFunc * ((|serializer|))
        serializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetSerializerMB(/* [in] */ const char *soapTypename,
                        /* [out] */ OpenSOAPSerializerFunc *serializer) {
    OpenSOAPStringPtr   tname = NULL;
    int ret = OpenSOAPStringCreateWithMB(soapTypename, &tname);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPGetSerializer(tname, serializer);

        OpenSOAPStringRelease(tname);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPGetSerializerWC(soapTypename, serializer)
    Get registered serializer.(WC)

    :Parameters
      :[in]  const wchar_t * ((|soapTypename|))
        typename.
      :[out] OpenSOAPSerializerFunc * ((|serializer|))
        serializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetSerializerWC(/* [in] */ const wchar_t *soapTypename,
                        /* [out] */ OpenSOAPSerializerFunc *serializer) {
    OpenSOAPStringPtr   tname = NULL;
    int ret = OpenSOAPStringCreateWithWC(soapTypename, &tname);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPGetSerializer(tname, serializer);

        OpenSOAPStringRelease(tname);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPGetDeserializer(soapTypename, deserializer)
    Get registered serializer.(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPStringPtr ((|soapTypename|))
        typename.
      :[out] OpenSOAPDeserializerFunc * ((|deserializer|))
        deserializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetDeserializer(/* [in] */  OpenSOAPStringPtr soapTypename,
                        /* [out] */ OpenSOAPDeserializerFunc *deserializer) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (soapTypename && deserializer) {
        StaticOpenSOAPSerializerPairPtr serializerPair = NULL;
        ret = OpenSOAPGetSerializerPair(soapTypename,
                                        &serializerPair);
        if (OPENSOAP_SUCCEEDED(ret)) {
            *deserializer = serializerPair->deserializer;
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPGetDeserializerMB(soapTypename, deserializer)
    Get registered serializer.(MB)

    :Parameters
      :[in]  const char * ((|soapTypename|))
        typename.
      :[out] OpenSOAPDeserializerFunc * ((|deserializer|))
        deserializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetDeserializerMB(/* [in] */ const char *soapTypename,
                          /* [out] */ OpenSOAPDeserializerFunc *deserializer) {
    OpenSOAPStringPtr   tname = NULL;
    int ret = OpenSOAPStringCreateWithMB(soapTypename, &tname);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPGetDeserializer(tname, deserializer);

        OpenSOAPStringRelease(tname);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPGetSerializerWC(soapTypename, deserializer)
    Get registered serializer.(WC)

    :Parameters
      :[in]  const wchar_t * ((|soapTypename|))
        typename.
      :[out] OpenSOAPDeserializerFunc * ((|deserializer|))
        deserializer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPGetDeserializerWC(/* [in] */  const wchar_t *soapTypename,
                          /* [out] */ OpenSOAPDeserializerFunc *deserializer) {
    OpenSOAPStringPtr   tname = NULL;
    int ret = OpenSOAPStringCreateWithWC(soapTypename, &tname);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPGetDeserializer(tname, deserializer);

        OpenSOAPStringRelease(tname);
    }

    return ret;
}

typedef struct {
    const wchar_t *soapTypename;
    OpenSOAPSerializerFunc serializer;
    OpenSOAPDeserializerFunc deserializer;
} OpenSOAPSerializerRegistInfo;

typedef OpenSOAPSerializerRegistInfo *OpenSOAPSerializerRegistInfoPtr;

static
OpenSOAPSerializerRegistInfo
SerializerRegisterInfos[] = {
	{ L"boolean",		OPENSOAP_SERIALIZER_PAIR(boolean)},
	{ L"base64Binary",	OPENSOAP_SERIALIZER_PAIR(base64Binary)},
	{ L"double", 		OPENSOAP_SERIALIZER_PAIR(double)},
	{ L"int", 			OPENSOAP_SERIALIZER_PAIR(int)},
	{ L"short", 		OPENSOAP_SERIALIZER_PAIR(short)},
	{ L"string", 		OPENSOAP_SERIALIZER_PAIR(string)},
	{ L"float", 		OPENSOAP_SERIALIZER_PAIR(float)},
	{ L"byte", 			OPENSOAP_SERIALIZER_PAIR(byte)},
	{ L"dateTime", 		OPENSOAP_SERIALIZER_PAIR(dateTime)},
    { NULL, NULL, NULL}
};
    
/*
=begin
--- function#OpenSOAPSerializerRegistDefaults()
    OpenSOAP default serializers/deserializers regist.
    :Parameters
	  nothing.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSerializerRegistDefaults(void) {
    int ret = OPENSOAP_NO_ERROR;
    OpenSOAPSerializerRegistInfoPtr i
        = SerializerRegisterInfos;
    for (;OPENSOAP_SUCCEEDED(ret) && i->soapTypename; ++i) {
        ret = OpenSOAPSerializerRegisterWC(i->soapTypename,
                                           i->serializer,
                                           i->deserializer);
    }

    return ret;
}

/*
--- function#OpenSOAPSerializerRegisterUltimate()
    OpenSOAP serializerTable release function.
    :Parameters
	  nothing.
    :Return Value
      :int
	    error code.
 */
extern
int
/* OPENSOAP_API */
OpenSOAPSerializerRegisterUltimate(void) {
	int ret = OpenSOAPStringHashApplyToValues(serializerTable,
											  deleteSerializerPair,
											  NULL);
	
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPStringHashRelease(serializerTable);
		if (OPENSOAP_SUCCEEDED(ret)) {
			serializerTable = NULL;
		}
	}

	return ret;
}
