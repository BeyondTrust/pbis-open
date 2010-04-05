#ifndef __LWIFILES_H__
#define __LWIFILES_H__

#define LWIFILES_CLIENT_GUID    "{AE472D6F-0615-4d12-BC70-8A381CA67D53}"
#define LWIFILES_ITEM_GUID      "{466BA033-CEEB-45fb-8ED4-4083B538AF76}"
#define LWIFILES_CACHEDIR   CACHEDIR "/" LWIFILES_CLIENT_GUID
#define LWIFILES_UNDOFILENAME   "lwi_gp_undo.xml"
#define LWIPOLICYSEQUENCEPATH LWIFILES_CACHEDIR "/lwipolicysequence.txt"

#define REMOVE_LINK_TAG             "remove-link"
#define CREATE_LINK_TAG             "create-link"
#define REMOVE_FILE_TAG             "remove-file"
#define MOVE_FILE_TAG               "move-file"
#define COPY_FILE_TAG               "copy-file"
#define REMOVE_DIR_TAG              "remove-dir"
#define CREATE_DIR_TAG              "create-dir"

#define ACTION_TAG                  "action"
#define UID_TAG                     "uid"
#define GID_TAG                     "gid"
#define MODE_TAG                    "mode"
#define TYPE_TAG                    "type"
#define DELETE_ON_POLICY_REVERT_TAG "DeleteOnPolicyRevert"

#define SAFE_FREE_ACTION_LIST(pActionList)  \
    if (pActionList) {                      \
        FreeActionList(pActionList);        \
    }

#define APPEND_TO_LIST(pActionListHead, pActionListTail, pActionList)   \
    if (pActionList) {                                                  \
        if (!pActionListHead) {                                         \
            pActionListHead = pActionList;                              \
            pActionListTail = pActionList;                              \
        }                                                               \
        else {                                                          \
            pActionListTail->pNext = pActionList;                       \
            pActionListTail = pActionList;                              \
        }                                                               \
        pActionList = NULL;                                             \
    }

typedef enum {
    UNSET = 0,
    REMOVE_FILE,
    COPY_FILE,
    MOVE_FILE,
    CREATE_DIRECTORY,
    REMOVE_DIRECTORY,
    CREATE_LINK,
    REMOVE_LINK,
    EXECUTE_COMMAND
} FileActionType;

typedef struct __FILESPOLICYACTION {
  FileActionType actionType;

    /*
        usage:
            a) Source path of a file or link being created
            b) Path of a file, folder or link being deleted
    */
    PSTR   pszSourcePath;

    /*
        usage:
            a) Path of a folder being created
            b) Target path of a file being copied, moved
            c) Target path of a link
    */
    PSTR   pszTargetPath;
    /* should we store the user name instead? */
    uid_t  uid;
    /* should we store the group name instead? */
    gid_t  gid;
    /* file system permissions */
    mode_t permissions;
    /* Should this object be deleted if the policy is reverted */
    BOOLEAN bDeleteOnRevert;
    /* Should this action be executed? */
    BOOLEAN bExecute;

    PSTR   pszPolicyIdentifier;

    /* command that has to be executed */
    PSTR   pszCommand;

    struct __FILESPOLICYACTION *pNext;
} FILESPOLICYACTION, *PFILESPOLICYACTION;

typedef struct __FILESPOLICY {
    PSTR pszPolicyIdentifier;
    PGROUP_POLICY_OBJECT pGPO;
    PGPOLWIGPITEM pGPItem;
    PGPOLWIDATA pLwidata;
    struct __FILESPOLICY *pNext;
} FILESPOLICY, *PFILESPOLICY;

static
CENTERROR
AddFilesPolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PFILESPOLICY * ppPolicy
    );

CENTERROR
ProcessFilesGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetFilesGroupPolicy(
    PGPUSER pUser
    );
 
#endif /* __LWIFILES_H__ */

