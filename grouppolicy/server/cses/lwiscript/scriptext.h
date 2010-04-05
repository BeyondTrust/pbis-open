#ifndef __SCRIPTEXT_H__
#define __SCRIPTEXT_H__

static const PSTR SCRIPT_TMP_FILE   = CACHEDIR "/likewise-script.new";
static const PSTR SCRIPT_DIRECTORY  = CACHEDIR "/";
static const PSTR CENTERIS_GP_DIRECTORY  = CACHEDIR "/";

#define LWISCRIPT_ITEM_GUID         "{04CD09B3-E969-4803-B8BD-6E6B5A945E92}"
#define LWISCRIPT_CLIENT_GUID       "{DDFF8E72-5C29-4987-8FB3-DF7EB7CE8FC2}"

typedef struct __POLICYLIST
{
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

#endif /* __SCRIPTEXT_H__ */

