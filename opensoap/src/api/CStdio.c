/*-----------------------------------------------------------------------------
 * $RCSfile: CStdio.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: CStdio.c,v 1.7 2003/11/20 07:03:18 okada Exp $";
#endif  /* _DEBUG */

#include "CStdio.h"

/*
=begin
= OpenSOAP CStdio class
=end
 */

#if defined(_MSC_VER)
#include <io.h>
#include <fcntl.h>

#define	dup(fd) _dup(fd)
#define fileno(fp) _fileno(fp)

static
const
int
OPENSOAP_FILE_BIN_MODE = _O_BINARY;

static
const
int
OPENSOAP_FILE_TXT_MODE = _O_TEXT;

static
int
OpenSOAPFILEPtrSetMode(FILE *fp,
                       int m) {
    int ret = OPENSOAP_NO_ERROR;
    if (fp) {
        int rslt = _setmode(_fileno(fp), m);
        if (rslt == -1) {
            ret = OPENSOAP_IO_ERROR;
        }
    }

    return ret;
}

#else /* !_MSC_VER */

#include <unistd.h>

static
const
int
OPENSOAP_FILE_BIN_MODE = 1;

static
const
int
OPENSOAP_FILE_TXT_MODE = 0;

#define OpenSOAPFILEPtrSetMode(fp,m) (OPENSOAP_NO_ERROR)

#endif /* !_MSC_VER */

/*
 */
static
int
OpenSOAPFILEPtrDuplicate(/* [in]  */ FILE *src,
						 /* [in]  */ const char *mode,
						 /* [out] */ FILE **dest) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (mode && *mode && dest) {
		if (!src) {
			*dest = NULL;
			ret = OPENSOAP_NO_ERROR;
		}
		else {
			int fd = fileno(src);
			ret = OPENSOAP_IO_ERROR;
			if (fd != -1) {
				fd = dup(fd);
				if (fd != -1) {
					*dest = fdopen(fd, mode);
					if (*dest) {
						ret = OPENSOAP_NO_ERROR;
					}
				}
			}
		}
	}

	return ret;
}

/*
 */
static
int
OpenSOAPCStdioReadImpl(/* [in]      */ OpenSOAPStreamPtr st,
                       /* [out]     */ unsigned char *buf,
                       /* [in, out] */ size_t *buf_sz) {
    OpenSOAPCStdioPtr s = (OpenSOAPCStdioPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (s && s->inputStream && buf && buf_sz && *buf_sz) {
        *buf_sz = fread(buf,
                        1,
                        *buf_sz,
                        s->inputStream);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPCStdioWriteImpl(/* [in]      */ OpenSOAPStreamPtr st,
                        /* [out]     */ const unsigned char *buf,
                        /* [in, out] */ size_t *buf_sz) {
    OpenSOAPCStdioPtr s = (OpenSOAPCStdioPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (s && s->outputStream && buf && buf_sz && *buf_sz) {
        *buf_sz = fwrite(buf,
                         1,
                         *buf_sz,
                         s->outputStream);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPCStdioFree(/* [out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPCStdioClose((OpenSOAPCStdioPtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStreamReleaseMembers((OpenSOAPStreamPtr)obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
            free(obj);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioCreate(s)
    OpenSOAP CStdio instance create

    :Parameters
      :OpenSOAPCStdioPtr * [out] ((|s|))
        OpenSOAP CStdio pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioCreate(/* [out] */ OpenSOAPCStdioPtr *s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (s) {
        *s = malloc(sizeof(OpenSOAPCStdio));
        if (*s) {
            ret = OpenSOAPStreamInitialize((OpenSOAPStreamPtr)*s,
                                           OpenSOAPCStdioFree,
                                           OpenSOAPCStdioReadImpl,
                                           OpenSOAPCStdioWriteImpl);
            if (OPENSOAP_FAILED(ret)) {
                free(*s);
                *s = NULL;
            }
            else {
                (*s)->inputStream = NULL;
                (*s)->outputStream = NULL;
            }
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioCreateWithFILE(s)
    OpenSOAP CStdio instance create

    :Parameters
      :OpenSOAPCStdioPtr * [out] ((|s|))
        OpenSOAP CStdio pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioCreateWithFILEPtr(/* [in]  */ FILE *is,
                                /* [in]  */ FILE *os,
                                /* [out] */ OpenSOAPCStdioPtr *s) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (s) {
		FILE *ifp = NULL;
		FILE *ofp = NULL;
		ret = OpenSOAPFILEPtrDuplicate(is, "r", &ifp);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPFILEPtrDuplicate(os, "w", &ofp);
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPCStdioCreate(s);
			if (OPENSOAP_SUCCEEDED(ret)) {
				(*s)->inputStream  = ifp;
				(*s)->outputStream = ofp;
			}
		}
		if (OPENSOAP_FAILED(ret)) {
			if (ifp) {
				fclose(ifp);
			}
			if (ofp) {
				fclose(ofp);
			}
		}
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioCreateForStdio(s)
    OpenSOAP CStdio instance create for stdio

    :Parameters
      :OpenSOAPCStdioPtr * [out] ((|s|))
        OpenSOAP CStdio pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioCreateForStdio(/* [out] */ OpenSOAPCStdioPtr *s) {
    int ret = OpenSOAPCStdioCreateWithFILEPtr(stdin, stdout, s);

    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioRelease(s)
    Release OpenSOAP CStdio

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioRelease(/* [in] */ OpenSOAPCStdioPtr s) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)s);
}

/*
=begin
--- function#OpenSOAPCStdioClose(s)
    Close and Disconnect OpenSOAP CStdio

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioClose(/* [in] */ OpenSOAPCStdioPtr s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        if (s->inputStream) {
			fclose(s->inputStream);
            s->inputStream = NULL;
        }
        if (s->outputStream) {
			fclose(s->outputStream);
            s->outputStream = NULL;
        }
        ret = OPENSOAP_NO_ERROR;
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioSetBinaryMode(s)
    Set OpenSOAP CStdio Stream to BINARY mode

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioSetBinaryMode(/* [in] */ OpenSOAPCStdioPtr s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        ret = OpenSOAPFILEPtrSetMode(s->inputStream,
                                     OPENSOAP_FILE_BIN_MODE);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPFILEPtrSetMode(s->outputStream,
                                         OPENSOAP_FILE_BIN_MODE);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioSetBinaryMode(s)
    Set OpenSOAP CStdio Stream to TEXT mode

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioSetTextMode(/* [in] */ OpenSOAPCStdioPtr s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        ret = OpenSOAPFILEPtrSetMode(s->inputStream,
                                     OPENSOAP_FILE_TXT_MODE);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPFILEPtrSetMode(s->outputStream,
                                         OPENSOAP_FILE_TXT_MODE);
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioRead(s, buf, buf_sz)
    Read From OpenSOAP CStdio Stream

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
      :unsigned char * [out] buf,
        Read Buffer
      :size_t * [in, out] buf_sz
        Read Buffer size. Returns actual size read.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioRead(/* [in]      */ OpenSOAPCStdioPtr s,
                   /* [out]     */ unsigned char *buf,
                   /* [in, out] */ size_t *buf_sz) {
    int ret = OpenSOAPStreamRead((OpenSOAPStreamPtr)s,
                                 buf,
                                 buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioWrite(s, buf, buf_sz)
    Write to OpenSOAP CStdio Stream

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
      :const unsigned char * [in] buf,
        Write Buffer
      :size_t * [in, out] buf_sz
        Write Buffer size. Returns actual size written
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioWrite(/* [in]      */ OpenSOAPCStdioPtr s,
                    /* [in]      */ const unsigned char *buf,
                    /* [in, out] */ size_t *buf_sz) {
    int ret = OpenSOAPStreamWrite((OpenSOAPStreamPtr)s,
                                  buf,
                                  buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPCStdioReadToDelm(s, delm, delm_sz, buf)
    Read to Delimiter from OpenSOAP CStdio Stream

    :Parameters
      :OpenSOAPCStdioPtr  [in] ((|s|))
        OpenSOAP CStdio
      :const unsigned char * [in] delm
        Delimiter string
      :size_t [in] delm_sz
        Delimiter length
      :OpenSOAPByteArrayPtr [out] buf
        Read Buffer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPCStdioReadToDelm(/* [in]  */ OpenSOAPCStdioPtr s,
                         /* [in]  */ const unsigned char *delm,
                         /* [in]  */ size_t delm_sz,
                         /* [out] */ OpenSOAPByteArrayPtr buf) {
    int ret = OpenSOAPStreamReadToDelm((OpenSOAPStreamPtr)s,
                                       delm,
                                       delm_sz,
                                       buf);

    return ret;
}
