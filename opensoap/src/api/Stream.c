/*-----------------------------------------------------------------------------
 * $RCSfile: Stream.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Stream.c,v 1.7.4.1 2004/06/04 07:23:10 okada Exp $";
#endif  /* _DEBUG */

#include "Stream.h"

/*
=begin
= OpenSOAP Stream class
=end
 */

extern
int
OPENSOAP_API
OpenSOAPStreamReleaseMembers(OpenSOAPStreamPtr s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        s->readFunc = NULL;
        s->writeFunc = NULL;
        ret = OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)s);
    }

    return ret;
}

extern
int
OPENSOAP_API
OpenSOAPStreamInitialize(OpenSOAPStreamPtr /* [in] */ s,
                         OpenSOAPObjectFreeFunc /* [in] */ free_func,
                         OpenSOAPStreamReadFunc  /* [in] */ read_func,
                         OpenSOAPStreamWriteFunc /* [in] */ write_func) {
    OpenSOAPObjectPtr obj = (OpenSOAPObjectPtr)s;
    int ret = OpenSOAPObjectInitialize(obj, free_func);
    if (OPENSOAP_SUCCEEDED(ret)) {
        s->readFunc  = read_func;
        s->writeFunc = write_func;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamRelease(s)
    Release OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamRelease(OpenSOAPStreamPtr /* [in, out] */ s) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)s);
}

/*
=begin
--- function#OpenSOAPStreamRead(s, buf, buf_sz)
    Read from OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :unsigned char * [out] buf,
        Start pointer of reading buffer
      :size_t * [in, out] buf_sz
        IN - Size of reading buffer.
        OUT - returns the size of data which have been actually read
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamRead(OpenSOAPStreamPtr /* [in] */ s,
                   unsigned char * /* [out] */ buf,
                   size_t * /* [in, out] */ buf_sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && s->readFunc) {
        ret = (s->readFunc)(s, buf, buf_sz);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamWrite(s, buf, buf_sz)
    Write to OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :const unsigned char * [in] ((|buf|))
        Start pointer of writing data
      :size_t * [in, out] buf_sz
        IN - Size of writing data.
        OUT - returns the size of data actually written
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamWrite(OpenSOAPStreamPtr /* [in] */ s,
                    const unsigned char * /* [in] */ buf,
                    size_t * /* [in, out] */ buf_sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && s->writeFunc) {
        ret = (s->writeFunc)(s, buf, buf_sz);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamReadToDelm(s, delm, delm_sz, buf)
    Read from OpenSOAP Stream To Delimiter

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :const unsigned char * [in] ((|delm|))
        string of Delimiter Characters
      :size_t [in] ((|delm_sz|))
        Length of Delimiter Characters
      :OpenSOAPByteArrayPtr [out] ((|buf|))
        Reading Buffer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamReadToDelm(OpenSOAPStreamPtr /* [in] */ s,
                         const unsigned char * /* [in] */ delm,
                         size_t /* [in] */ delm_sz,
                         OpenSOAPByteArrayPtr /* [out] */ buf) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (delm && delm_sz) {
        ret = OpenSOAPByteArrayClear(buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char * const delm_end = delm + delm_sz;
            const unsigned char *delm_i = delm;
        
            while (delm_i != delm_end) {
                unsigned char read_dat = '\x0';
                size_t read_sz = 1;
                ret =
                    OpenSOAPStreamRead(s, &read_dat, &read_sz);
                if (OPENSOAP_FAILED(ret) || !read_sz) {
                    break;
                }
                ret =
                    OpenSOAPByteArrayAppend(buf,
                                            &read_dat,
                                            1);
                if (OPENSOAP_FAILED(ret)) {
                    break;
                }
                if (*delm_i == read_dat) {
                    ++delm_i;
                }
                else {
                    delm_i = delm;
                }
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamGetChunkedSize(s)
    Read and get a size of Chunked Data as HEX from OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :size_t* [out] ((|size|))
        Chunked data size
    :Returns
      :int
              Error Code
=end
 */
static
int
OpenSOAPStreamGetChunkedSize(OpenSOAPStreamPtr /* [in] */ s,
                             size_t /* [out] */ * size)
{
    unsigned char * ch = NULL;
    char c;
    int ch_sz;
    int ret;
    OpenSOAPByteArrayPtr buf = NULL;

    *size = 0;
    ret = OpenSOAPByteArrayCreate(&buf);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStreamReadToDelm(s, "\r\n", 2, buf);
    }
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPByteArrayBegin(buf, &ch);
    }
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPByteArrayGetSize(buf, &ch_sz);
    }
    if (OPENSOAP_SUCCEEDED(ret)) {
        ch[ch_sz - 2] = '\0';
        /* convert HEX character string to an integer value */
        for(; *ch != '\0' && *ch != ' ' ; ch++) {
            c = *ch;
            if ('A'<=c && c<='F') {
                c -= 'A' - '9' - 1;
            }
            if ('a'<=c && c<='f') {
                c -= 'a' - '9' - 1;
            }
            c -= '0';
            /* check invalid characters */
            if (0 <= c && c < 16) {
                *size = (*size)*16 + (size_t)c;
            }
        }
        while(*ch != '\0') {
            ch++;
        }
    }
    OpenSOAPByteArrayRelease(buf);

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamReadChunkedDataStream(s, out)
    Read Chunked Data from OpenSOAP Stream and write out to Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :OpenSOAPStreamPtr [out] ((|out|))
        Output Stream
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamReadChunkedDataStream(OpenSOAPStreamPtr /* [in] */ s,
                                    OpenSOAPStreamPtr /* [out] */ out) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
#define READ_BUF_SZ 128
        unsigned char read_buf[READ_BUF_SZ];
        size_t read_sum_sz = 0;
        size_t sz = 0;
        while (OPENSOAP_SUCCEEDED(ret)) {
            size_t read_sz;
            /* Get Chunked Data */
            if (sz == 0) {
                ret = OpenSOAPStreamGetChunkedSize(s, &sz);
                if (OPENSOAP_FAILED(ret) || sz == 0) {
                    break;
                }
            }
            if (sz && sz < read_sum_sz + READ_BUF_SZ) {
                read_sz = sz - read_sum_sz;
            }
            else {
                read_sz = READ_BUF_SZ;
            }
            /*
              fprintf(stderr, "sz:[%d],read_sum_sz:[%d],read_sz[%d]\n", sz,
              read_sum_sz, read_sz);
            */
            if (!read_sz) {
                break;
            }
            ret = OpenSOAPStreamRead(s,
                                     read_buf,
                                     &read_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (!read_sz) {
                    break;
                }
                ret = OpenSOAPStreamWrite(out,
                                          read_buf,
                                          &read_sz);
                read_sum_sz += read_sz;
            }
        }
#undef READ_BUF_SZ
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamReadChunkedData(s, buf)
    Read Chunked Data from OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :OpenSOAPByteArrayPtr [out] ((|buf|))
        Reading Buffer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamReadChunkedData(OpenSOAPStreamPtr /* [in] */ s,
                              OpenSOAPByteArrayPtr /* [out] */ buf) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
#define READ_BUF_SZ 128
        unsigned char read_buf[READ_BUF_SZ];
        size_t read_sum_sz = 0;
        size_t sz = 0;
        ret = OpenSOAPByteArrayClear(buf);
        while (OPENSOAP_SUCCEEDED(ret)) {
            size_t read_sz;
            /* Get Chunked Data */
            if (sz == 0) {
                ret = OpenSOAPStreamGetChunkedSize(s, &sz);
                if (OPENSOAP_FAILED(ret) || sz == 0) {
                    break;
                }
            }
            if (sz && sz < read_sum_sz + READ_BUF_SZ) {
                read_sz = sz - read_sum_sz;
            }
            else {
                read_sz = READ_BUF_SZ;
            }
            /*
              fprintf(stderr, "sz:[%d],read_sum_sz:[%d],read_sz[%d]\n", sz,
              read_sum_sz, read_sz);
            */
            if (!read_sz) {
                break;
            }
            ret = OpenSOAPStreamRead(s,
                                     read_buf,
                                     &read_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (!read_sz) {
                    break;
                }
                ret = OpenSOAPByteArrayAppend(buf,
                                              read_buf,
                                              read_sz);
                read_sum_sz += read_sz;
            }
        }
#undef READ_BUF_SZ
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamReadWithSizeStream(s, sz, out)
    Read Specified Size from OpenSOAP Stream
    if sz is 0, read all data

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :size_t [in] ((|sz|))
        Reading Size
      :OpenSOAPStreamPtr [out] ((|out|))
        Output Stream
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamReadWithSizeStream(OpenSOAPStreamPtr /* [in] */ s,
                           size_t /* [in] */ sz,
                           OpenSOAPStreamPtr /* [out] */ out) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (s) {
#define READ_BUF_SZ 128
        unsigned char read_buf[READ_BUF_SZ];
        size_t read_sum_sz = 0;
        ret = OPENSOAP_NO_ERROR;
        while (OPENSOAP_SUCCEEDED(ret)) {
            size_t read_sz
                = (sz && sz < read_sum_sz + READ_BUF_SZ)
                ? (sz - read_sum_sz) : READ_BUF_SZ;
            if (!read_sz) {
                break;
            }
            ret = OpenSOAPStreamRead(s,
                                     read_buf,
                                     &read_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (!read_sz) {
                    break;
                }
                ret = OpenSOAPStreamWrite(out,
                                          read_buf,
                                          &read_sz); 
                read_sum_sz += read_sz;
            }
        }
#undef READ_BUF_SZ
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamReadWithSize(s, sz, buf)
    Read Specified Size from OpenSOAP Stream
    if sz is 0, read all data

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :size_t [in] ((|sz|))
        Reading Size
      :OpenSOAPByteArrayPtr [out] ((|buf|))
        Reading Buffer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamReadWithSize(OpenSOAPStreamPtr /* [in] */ s,
                           size_t /* [in] */ sz,
                           OpenSOAPByteArrayPtr /* [out] */ buf) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (s) {
#define READ_BUF_SZ 128
        unsigned char read_buf[READ_BUF_SZ];
        size_t read_sum_sz = 0;
        ret = OpenSOAPByteArrayClear(buf);
        while (OPENSOAP_SUCCEEDED(ret)) {
            size_t read_sz
                = (sz && sz < read_sum_sz + READ_BUF_SZ)
                ? (sz - read_sum_sz) : READ_BUF_SZ;
            if (!read_sz) {
                break;
            }
            ret = OpenSOAPStreamRead(s,
                                     read_buf,
                                     &read_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (!read_sz) {
                    break;
                }
                ret = OpenSOAPByteArrayAppend(buf,
                                              read_buf,
                                              read_sz);
                read_sum_sz += read_sz;
            }
        }
#undef READ_BUF_SZ
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStreamWriteByteArray(s, buf)
    Write to OpenSOAP Stream

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|s|))
        OpenSOAP Stream
      :OpenSOAPByteArrayPtr [in] ((|buf|))
        Data to be written
      :size_t * [out] ((|write_sz|))
        Size to be written
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPStreamWriteByteArray(OpenSOAPStreamPtr /* [in] */ s,
                             OpenSOAPByteArrayPtr /* [in] */ buf,
                             size_t * /* [out] */ write_sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    const unsigned char *buf_beg = NULL;
    size_t sz = 0;

    if (!write_sz) {
        write_sz = &sz;
    }
    ret = OpenSOAPByteArrayGetBeginSizeConst(buf, &buf_beg, write_sz);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStreamWrite(s, buf_beg, write_sz);
    }

    return ret;
}
