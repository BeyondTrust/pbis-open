/*-----------------------------------------------------------------------------
 * $RCSfile: StringHash.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: StringHash.c,v 1.22 2003/01/22 08:21:44 bandou Exp $";
#endif  /* _DEBUG */

#include "StringHash.h"

#include <string.h>

/*
=begin
= StringHash.c
OpenSOAPStringHash Implement file.
=end
*/

#define INITIAL_ALLOC_SIZE (256)
#define GROW_ALLOC_SIZE (128)

/*
=begin
--- function#OpenSOAPStringHashReleaseMembers(strHash)
    OpenSOAPStringHash's member release.
    :Parameters
      :[in]  OpenSOAPStringHashPtr ((|strHash|))
    :Returns
      :int
	    Error code.
=end
 */
static
int
OpenSOAPStringHashReleaseMembers(OpenSOAPStringHashPtr strHash) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strHash) {
        OpenSOAPStringHashClear(strHash);
        free(strHash->hash);
        ret = OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)strHash);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashFree(obj)
    Free OpenSOAPStringHash.
    :Parameters
      :[in]  OpenSOAPObjectPtr ((|obj|))
    :Returns
      :int
	    Error code.
=end
 */
static
int
OpenSOAPStringHashFree(OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        OpenSOAPStringHashPtr strHash = (OpenSOAPStringHashPtr)obj;

        ret = OpenSOAPStringHashReleaseMembers(strHash);
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashInitialize(strHash)
    OpenSOAPStringHash initialize.
	
    :Parameters
      :OpenSOAPStringHashPtr [IN] ((|strHash|))
    :Returns
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
 */
static
int
OpenSOAPStringHashInitialize(OpenSOAPStringHashPtr strHash) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strHash) {
        ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)strHash,
                                       OpenSOAPStringHashFree);
        if (OPENSOAP_SUCCEEDED(ret)) {
            strHash->hashSize = 0;
            strHash->hashAllocSize = INITIAL_ALLOC_SIZE;
            strHash->hash = malloc(strHash->hashAllocSize
                                    * sizeof(OpenSOAPStringHashPair));
            if (!strHash->hash) {
                OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)strHash);
                ret = OPENSOAP_MEM_BADALLOC;
            }
            else {
                memset(strHash->hash, 0,
                       strHash->hashAllocSize
                       * sizeof(OpenSOAPStringHashPair));
            }
        }
    }

    return ret;
}


/*
=begin
--- function#OpenSOAPStringHashCreate(strh)
    Create OpenSOAPString Hash
    
    :Parameters
       :[out] OpenSOAPStringHashPtr * ((|strh|))
         作成した OpenSOAP 文字列 Hash のポインタの格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashCreate(/* [out] */ OpenSOAPStringHashPtr *strh) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh) {
        ret = OPENSOAP_MEM_BADALLOC;
        *strh = malloc(sizeof(OpenSOAPStringHash));
        if (*strh) {
            ret = OpenSOAPStringHashInitialize(*strh);
            if (OPENSOAP_FAILED(ret)) {
                free(*strh);
                *strh = NULL;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashRelease(strh)
    Release OpenSOAPString Hash
    
    :Parameters
       :[in]  OpenSOAPStringHashPtr ((|strh|))
         OpenSOAPString Hash.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashRelease(/* [in] */ OpenSOAPStringHashPtr strh) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)strh);

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashClear(strh)
    Clear OpenSOAPString Hash
    
    :Parameters
       :[in, out] OpenSOAPStringHashPtr ((|strh|))
         OpenSOAPString Hash.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashClear(/* [in, out] */ OpenSOAPStringHashPtr strh) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh) {
        OpenSOAPStringHashPairPtr realloc_hash = NULL;
        OpenSOAPStringHashPairPtr beg = strh->hash;
        OpenSOAPStringHashPairPtr ed = beg + strh->hashSize;
        OpenSOAPStringHashPairPtr i = beg;
        for (; i != ed; ++i) {
            OpenSOAPStringRelease(i->key);
        }
        realloc_hash = realloc(strh->hash, INITIAL_ALLOC_SIZE);
        if (realloc_hash) {
            strh->hash = realloc_hash;
            strh->hashAllocSize = INITIAL_ALLOC_SIZE;
        }
        strh->hashSize = 0;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashCalcHash(strh, key, hash_val)
    Calculate hash value.
	
    :Parameters
      :[in]  OpenSOAPStringHashPtr ((|strh|))
      :[in]  OpenSOAPStringPtr ((|key|))
      :[out] size_t * ((|hash_val|))
    :Returns
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
 */
static
int
OpenSOAPStringHashCalcHash(/* [in]  */ OpenSOAPStringHashPtr strh,
                           /* [in]  */ OpenSOAPStringPtr key,
                           /* [out] */ size_t *hash_val) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh && key && hash_val) {
        OpenSOAPStringHashPairPtr beg = strh->hash;
        OpenSOAPStringHashPairPtr ed = beg + strh->hashSize;
        OpenSOAPStringHashPairPtr i = beg;
        for (; i != ed; ++i) {
            int judge = 0;
            ret = OpenSOAPStringCompare(i->key, key, &judge);
            if (OPENSOAP_SUCCEEDED(ret) && judge == 0) {
                break;
            }
        }
        *hash_val = i - beg;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashRemoveKey(strh, key, val)
    OpenSOAPString Hash から登録削除。
    
    :Parameters
       :OpenSOAPStringHashPtr [in, out] ((|strh|))
         OpenSOAPString Hash.
       :OpenSOAPStringPtr [in] ((|key|))
         キー値
       :void ** [out] ((|val|))
         登録していた値の格納場所。NULL の場合は値を返さない。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashRemoveKey(/* [in, out] */ OpenSOAPStringHashPtr strh,
                            /* [in]  */ OpenSOAPStringPtr key,
                            /* [out] */ void **val) {
    size_t hash_val = 0;
    int ret = OpenSOAPStringHashCalcHash(strh, key, &hash_val);

    if (OPENSOAP_SUCCEEDED(ret)) {
        void *val_dummy = NULL;
        if (!val) {
            val = &val_dummy;
        }
        if (hash_val < strh->hashSize) {
            OpenSOAPStringHashPairPtr pair = strh->hash + hash_val;
            *val = pair->value;
            OpenSOAPStringRelease(pair->key);
            memmove(pair, pair + 1,
                    (strh->hashSize - hash_val)
                    * sizeof(OpenSOAPStringHashPair));
            --(strh->hashSize);
        }
    }
    
    return ret;
}
    
/*
=begin
--- function#OpenSOAPStringHashSetValueWithFlag(strh, key, val, is_dup)
    OpenSOAPString Hash に値の登録
    
    :Parameters
       :OpenSOAPStringHashPtr [in, out] ((|strh|))
         OpenSOAPString Hash.
       :OpenSOAPStringPtr [in] ((|key|))
         キー値
       :void * [in] ((|val|))
         登録する値。
       :int [in] ((|is_dup|))
         key を複製して設定するかどうかのフラグ
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
*/
static
int
/* OPENSOAP_API */
OpenSOAPStringHashSetValueWithFlag(/* [in, out] */ OpenSOAPStringHashPtr strh,
                                   /* [in]      */ OpenSOAPStringPtr key,
                                   /* [in]      */ void *val,
                                   /* [in]      */ int is_dup) {
    size_t hash_val = 0;
    int ret = OpenSOAPStringHashCalcHash(strh, key, &hash_val);

    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStringHashPairPtr hash = strh->hash + hash_val;
        if (hash_val >= strh->hashSize) {
            if (hash_val >= strh->hashAllocSize) {
                size_t realloc_size = strh->hashAllocSize + GROW_ALLOC_SIZE;
                OpenSOAPStringHashPairPtr realloc_mem
                    = realloc(strh->hash,
                              realloc_size * sizeof(OpenSOAPStringHashPair));
                if (realloc_mem) {
                    memset(realloc_mem + strh->hashAllocSize, 0,
                           (realloc_size - strh->hashAllocSize)
                           * sizeof(OpenSOAPStringHashPair));
                    strh->hash = realloc_mem;
                    strh->hashAllocSize = realloc_size;
                    hash = strh->hash + hash_val;
                }
                else {
                    ret = OPENSOAP_MEM_BADALLOC;
                }
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                ++(strh->hashSize);
                if (is_dup) {
                    ret = OpenSOAPStringDuplicate(key,
                                                  &hash->key);
                }
                else {
                    hash->key = key;
                }
            }
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            hash->value = val;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashSetValue(strh, key, val)
    OpenSOAPString Hash に値の登録
    
    :Parameters
       :OpenSOAPStringHashPtr [in, out] ((|strh|))
         OpenSOAPString Hash.
       :OpenSOAPStringPtr [in] ((|key|))
         キー値
       :void * [in] ((|val|))
         登録する値。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashSetValue(/* [in, out] */ OpenSOAPStringHashPtr strh,
                           /* [in]      */ OpenSOAPStringPtr key,
                           /* [in]      */ void *val) {
    int ret = OpenSOAPStringHashSetValueWithFlag(strh, key, val, 1);

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashSetValueMB(strh, key, val)
    OpenSOAPString Hash に値の登録
    
    :Parameters
       :OpenSOAPStringHashPtr [in, out] ((|strh|))
         OpenSOAPString Hash.
       :const char * [in] ((|key|))
         キー値
       :void * [in] ((|val|))
         登録する値。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashSetValueMB(/* [in, out] */ OpenSOAPStringHashPtr strh,
                             /* [in]      */ const char *key,
                             /* [in]      */ void *val) {
    OpenSOAPStringPtr key_str = NULL;
    int ret = OpenSOAPStringCreateWithMB(key, &key_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringHashSetValueWithFlag(strh, key_str, val, 0);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashSetValueWC(strh, key, val)
    OpenSOAPString Hash に値の登録
    
    :Parameters
       :OpenSOAPStringHashPtr [in, out] ((|strh|))
         OpenSOAPString Hash.
       :const wchar_t * [in] ((|key|))
         キー値
       :void * [in] ((|val|))
         登録する値。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashSetValueWC(/* [in, out] */ OpenSOAPStringHashPtr strh,
                             /* [in]      */ const wchar_t *key,
                             /* [in]      */ void *val) {
    OpenSOAPStringPtr key_str = NULL;
    int ret = OpenSOAPStringCreateWithWC(key, &key_str);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringHashSetValueWithFlag(strh, key_str, val, 0);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetValue(strh, key, val)
    OpenSOAPString Hash から値の取得
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :OpenSOAPStringPtr [in] ((|key|))
         キー値
       :void ** [out] ((|val|))
         取得する値の格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetValue(/* [in]  */ OpenSOAPStringHashPtr strh,
                           /* [in]  */ OpenSOAPStringPtr key,
                           /* [out] */ void **val) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (val) {
        size_t hash_val = 0;
        ret = OpenSOAPStringHashCalcHash(strh, key, &hash_val);
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (hash_val >= strh->hashSize) {
                ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
            }
            else {
                *val = strh->hash[hash_val].value;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetValueMB(strh, key, val)
    OpenSOAPString Hash から値の取得
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :const char * [in] ((|key|))
         キー値
       :void ** [out] ((|val|))
         取得する値の格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetValueMB(/* [in]  */ OpenSOAPStringHashPtr strh,
                             /* [in]  */ const char *key,
                             /* [out] */ void **val) {
    OpenSOAPStringPtr key_str = NULL;
    int ret = OpenSOAPStringCreateWithMB(key, &key_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringHashGetValue(strh, key_str, val);
        OpenSOAPStringRelease(key_str);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetValueWC(strh, key, val)
    OpenSOAPString Hash から値の取得
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :const wchar_t * [in] ((|key|))
         キー値
       :void ** [out] ((|val|))
         取得する値の格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetValueWC(/* [in]  */ OpenSOAPStringHashPtr strh,
                             /* [in]  */ const wchar_t *key,
                             /* [out] */ void **val) {
    OpenSOAPStringPtr key_str = NULL;
    int ret = OpenSOAPStringCreateWithWC(key, &key_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStringHashGetValue(strh, key_str, val);
        OpenSOAPStringRelease(key_str);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetSize(strh, sz)
    OpenSOAPString Hash の登録数の取得。
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :size_t * [out] ((|sz|))
         取得する登録数の格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetSize(/* [in]  */ OpenSOAPStringHashPtr strh,
                          /* [out] */ size_t *sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh && sz) {
        *sz = strh->hashSize;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetKeys(strh, sz, keys)
    OpenSOAPString Hash の登録Key全ての取得
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :size_t * [out] ((|sz|))
         [in]  取得する登録Keyの格納場所のサイズ。
         [out] key に格納したサイズ。
       :OpenSOAPStringPtr * [out] ((|keys|))
         取得する登録Keyの格納場所。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetKeys(/* [in]      */ OpenSOAPStringHashPtr strh,
                          /* [in, out] */ size_t *sz,
                          /* [out]     */ OpenSOAPStringPtr *keys) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh && sz && keys) {
        if (strh->hashSize >= *sz) {
            OpenSOAPStringPtr *dist = keys;
            OpenSOAPStringHashPairPtr i = strh->hash;
            OpenSOAPStringHashPairPtr ed = i + strh->hashSize;
            for (ret = OPENSOAP_NO_ERROR;
                 i != ed && OPENSOAP_SUCCEEDED(ret); ++i, ++dist) {
                *dist = NULL;
                ret = OpenSOAPStringDuplicate(i->key, dist);
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPStringPtr *r = keys;
                for (r = keys; r != dist; ++r) {
                    OpenSOAPStringRelease(*r);
                }
            }
            else {
                *sz = dist - keys;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashGetValues(strh, sz, vals)
    OpenSOAPString Hash の登録値全ての取得
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :size_t * [out] ((|sz|))
         [in]  取得する登録Keyの格納場所のサイズ。
         [out] key の数。
       :void ** [out] ((|vals|))
         取得する登録値の格納場所。値の順番は OpenSOAPStringHashGetKeys 
         関数で取得した key の順番とは無関係とする。
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashGetValues(/* [in]      */ OpenSOAPStringHashPtr strh,
                            /* [in, out] */ size_t *sz,
                            /* [out]     */ void **vals) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (strh && sz && vals) {
        if (strh->hashSize >= *sz) {
            void **dist = vals;
            OpenSOAPStringHashPairPtr i = strh->hash;
            OpenSOAPStringHashPairPtr ed = i + strh->hashSize;
            
            for (; i != ed; ++i, ++dist) {
                *dist = i->value;
            }
            *sz = dist - vals;
            ret = OPENSOAP_NO_ERROR;
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPStringHashApplyToValues(strh, aply, opt)
    OpenSOAPString Hash の登録値全てへの関数の適用。
    
    :Parameters
       :OpenSOAPStringHashPtr [in] ((|strh|))
         OpenSOAPString Hash.
       :int [in] ( * ((|aply|)) )(void *val, void *opt)
         適用する関数。この関数の戻り値を ret として、
         OPENSOAP_FAILED(ret) が真になるかまたは、
         全ての値にこの関数を適用するまで続ける。
       :void * [in] ((|opt|))
         適用する関数のオプションパラメータ。
    :Return Value
      :int
	    Return Code Is ErrorCode
    :example
      int
      sumHash(void *val, void *opt) {
          int *sum = opt;
          int iVal = (int)val;

          *sum += iVal;

          return OPENSOAP_NO_ERROR;
      }

      ...

      OpenSOAPStringHashPtr strHash = NULL;
      int ret = OpenSOAPStringHashCreate(&strHash);

      ...
      int sum = 0;
      int ret = OpenSOAPStringHashApplyToValues(strHash, sumHash, &sum);
      
      if (OPENSOAP_SUCCEEDED(ret)) {
        printf("sum = %d\n", sum);
      }
      
=end
*/
int
OPENSOAP_API
OpenSOAPStringHashApplyToValues(/* [in, out] */ OpenSOAPStringHashPtr strh,
                                /* [in]      */ int  (*aply)(void *, void *),
                                /* [in]      */ void *opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (strh && aply) {
        OpenSOAPStringHashPairPtr i = strh->hash;
        OpenSOAPStringHashPairPtr ed = i + strh->hashSize;
        for (ret = OPENSOAP_NO_ERROR;
             i != ed && OPENSOAP_SUCCEEDED(ret); ++i) {
            ret = aply(i->value, opt);
        }
    }

    return ret;
}

