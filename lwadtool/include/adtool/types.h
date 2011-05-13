/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        types.h
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#ifndef _ADTOOL_TYPES_H_
#define _ADTOOL_TYPES_H_

/**
 * Action codes.
 */
typedef enum
{
  AdtBaseAction = 0,

  /**
   * Enterprise edition.
   */
  AdtNewCellAction,
  AdtEditCellAction,
  AdtEditCellUserAction,
  AdtEditCellGroupAction,
  AdtAddToCellAction,
  AdtRemoveFromCellAction,
  AdtLinkCellAction,
  AdtUnlinkCellAction,
  AdtSearchCellsAction,
  AdtLookupCellAction,
  AdtLookupCellUserAction,
  AdtLookupCellGroupAction,
  AdtDeleteCellAction,

  /**
   * Open edition.
   */
  AdtDeleteObjectAction,
  AdtMoveObjectAction,
  AdtNewUserAction,
  AdtNewGroupAction,
  AdtNewOuAction,
  AdtNewComputerAction,
  AdtSearchUserAction,
  AdtSearchGroupAction,
  AdtSearchOuAction,
  AdtSearchComputerAction,
  AdtSearchObjectAction,
  AdtLookupObjectAction,
  AdtEnableUserAction,
  AdtEnableComputerAction,
  AdtDisableUserAction,
  AdtDisableComputerAction,
  AdtResetUserPasswordAction,
  AdtResetComputerPasswordAction,
  AdtAddToGroupAction,
  AdtRemoveFromGroupAction,
  AdtUnlockAccountAction
} AdtActionCode;

/**
 * Action data types. TODO: Add LW Open actions.
 */
typedef struct AdtActionBase {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
} AdtActionBaseT, *AdtActionBaseTP;

typedef struct AdtActionNewCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR homeDirTemplate; /* Default home dir template: %H/%D/%U */
    PSTR defaultShell; /* Default login shell: /bin/bash */
} AdtActionNewCellT, *AdtActionNewCellTP;

typedef struct AdtActionEditCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR homeDirTemplate; /* Default home dir template: %H/%D/%U */
    PSTR defaultShell; /* Default login shell: /bin/bash */
} AdtActionEditCellT, *AdtActionEditCellTP;

typedef struct AdtActionEditCellUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR gid; /* User's primary group ID */
    PSTR desc; /* Description of the user */
    PSTR homeDir; /* User's home directory */
    PSTR loginShell; /* User's login shell */
    PSTR user; /* User to change cell properties of (DN/RDN, UPN, or SamAccountName) */
    PSTR uid; /* User's ID */
    PSTR loginName; /* User's login name */
} AdtActionEditCellUserT, *AdtActionEditCellUserTP;

typedef struct AdtActionEditCellGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR gid; /* Group's ID */
    PSTR desc; /* Group description */
    PSTR homeDir; /* User's home directory */
    PSTR loginShell; /* User's login shell */
    PSTR group; /* Group to change cell properties of (DN/RDN, or CN) */
    PSTR alias; /* Group's alias */
} AdtActionEditCellGroupT, *AdtActionEditCellGroupTP;

typedef struct AdtActionDeleteCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    INT isDeleteMembers; /* Delete cell members. Fail by default if the cell is not empty. */
} AdtActionDeleteCellT, *AdtActionDeleteCellTP;

typedef struct AdtActionAddToCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn;      /* OU containing the cell (DN/RDN) */
    PSTR user;    /* User to be added to the cell (DN/RDN, UPN, or SamAccountName) */
    PSTR group;   /* Group to be added to the cell (DN/RDN, or CN) */
} AdtActionAddToCellT, *AdtActionAddToCellTP;

typedef struct AdtActionRemoveFromCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn;      /* OU containing the cell (DN/RDN) */
    PSTR user;    /* User to be removed from the cell (DN/RDN, UPN, or SamAccountName) */
    PSTR group;   /* Group to be removed from the cell (DN/RDN, or CN) */
    INT isUnsetAttrs; /* Unset values of uidNumber, gidNumber, loginShell, and unixHomeDirectory attributes when operating on a default LW cell. */
} AdtActionRemoveFromCellT, *AdtActionRemoveFromCellTP;

typedef struct AdtActionLinkCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR sourceCell; /* OU containing the source cell (DN/RDN) */
    PSTR targetCell; /* OU containing the target cell (DN/RDN) */
} AdtActionLinkCellT, *AdtActionLinkCellTP;

typedef struct AdtActionUnlinkCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR sourceCell; /* OU containing the source cell (DN/RDN) */
    PSTR targetCell; /* OU containing the target cell (DN/RDN) */
} AdtActionUnlinkCellT, *AdtActionUnlinkCellTP;

typedef struct AdtActionSearchCells {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;  /* Search base (DN of top-level node to start the search from) */
    PSTR scope; /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN; /* Numeric value of the scope */
    PSTR user;  /* Search for cells the user is a member of (DN/RDN, UPN, or SamAccountName) */
    PSTR group; /* Search for cells the group is a member of (DN/RDN, or CN) */
} AdtActionSearchCellT, *AdtActionSearchCellTP;

typedef struct AdtActionLookupCell {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    INT isHomeDirTemplate; /* Retrieve the current home dir template */
    INT isDefaultShell; /* Retrieve the default login shell */
    INT isUsers; /* Retrieve users defined in the cell */
    INT isGroups; /* Retrieve groups defined in the cell */
    INT isLinkedCells; /* Retrieve names of the linked cells */
    INT isAll; /* Retrieve all properties */
} AdtActionLookupCellT, *AdtActionLookupCellTP;

typedef struct AdtActionLookupCellUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR user; /* User to look up cell properties of (DN/RDN, UPN, or SamAccountName) */
    INT isUid; /* Retrieve user's ID */
    INT isGid; /* Retrieve user's primary group ID */
    INT isLoginName; /* Retrieve user's login name */
    INT isHomeDir; /* Retrieve user's home directory */
    INT isLoginShell; /* Retrieve user's login shell */
    INT isComment; /* Retrieve description of the user */
    INT isAll; /* Retrieve all properties */
} AdtActionLookupCellUserT, *AdtActionLookupCellUserTP;

typedef struct AdtActionLookupCellGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN of the OU containing the cell */
    PSTR group; /* Retrieve group to look up cell properties of (DN/RDN, or CN) */
    INT isGid; /* Retrieve group's ID */
    INT isAlias; /* Retrieve group's alias */
    INT isDesc; /* Retrieve group description */
    INT isAll; /* Retrieve all properties */
} AdtActionLookupCellGroupT, *AdtActionLookupCellGroupTP;

typedef struct AdtActionDeleteObject {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn; /* DN/RDN of the object to delete */
    INT isDeleteMembers; /* Delete cell members. Fail by default if the cell is not empty. */
} AdtActionDeleteObjectT, *AdtActionDeleteObjectTP;

typedef struct AdtActionMoveObject {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR from; /* DN/RDN of the object to move */
    PSTR to; /* New DN/RDN of the object */
} AdtActionMoveObjectT, *AdtActionMoveObjectTP;

typedef struct AdtActionSearchUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;  /* Search base (DN of top-level node to start the search from) */
    PSTR scope; /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN; /* Numeric value of the scope */
    PSTR name;  /* User to search for (DN/RDN, UPN, or SamAccountName) */
} AdtActionSearchUserT, *AdtActionSearchUserTP;

typedef struct AdtActionSearchGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;  /* Search base (DN of top-level node to start the search from) */
    PSTR scope; /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN; /* Numeric value of the scope */
    PSTR name;  /* Group to search for (DN/RDN, UPN, or SamAccountName) */
} AdtActionSearchGroupT, *AdtActionSearchGroupTP;

typedef struct AdtActionSearchOu {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;  /* Search base (DN of top-level node to start the search from) */
    PSTR scope; /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN; /* Numeric value of the scope */
    PSTR name;  /* Group to search for (DN/RDN, or CN) */
} AdtActionSearchOuT, *AdtActionSearchOuTP;

typedef struct AdtActionSearchComputer {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;  /* Search base (DN of top-level node to start the search from) */
    PSTR scope; /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN; /* Numeric value of the scope */
    PSTR name;  /* Group to search for (DN/RDN, UPN, or SamAccountName) */
} AdtActionSearchComputerT, *AdtActionSearchComputerTP;

typedef struct AdtActionSearchObject {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR base;    /* Search base (DN of top-level node to start the search from) */
    PSTR scope;   /* Search scope. Acceptable values: base, one-level, subtree (default) */
    INT scopeN;   /* Numeric value of the scope */
    PSTR filter;  /* Search filter */
} AdtActionSearchObjectT, *AdtActionSearchObjectTP;

typedef struct AdtActionLookupObject {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn;      /* DN/RDN of the object */
    PSTR attr;    /* Attribute to look up */
    INT isAll;    /* Retrieve all properties */
} AdtActionLookupObjectT, *AdtActionLookupObjectTP;

typedef struct AdtActionNewOu {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn;      /* DN/RDN of the parent */
    PSTR name;    /* OU name */
    PSTR desc;    /* Description */
} AdtActionNewOuT, *AdtActionNewOuTP;

typedef struct AdtActionNewUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR dn;                  /* DN/RDN of the parent */
    PSTR name;                /* samAccountName */
    PSTR desc;                /* Description */
    PSTR namePreWin2000;      /* Pre Windows-2000 name (samAccountName) */
    PSTR cn;                  /* CN */
    PSTR nameFirst;           /* First name */
    PSTR nameLast;            /* Last name */
    PSTR password;            /* Password */
    INT isNoMustChangePasswd; /* If set user does not need to change password at next logon */
    INT isNoCanChangePasswd;  /* User is not allowed to change password if set */
    INT isNoPasswdExpires;    /* Password never expires if set */
    INT isAccountEnabled;     /* Account is enabled if set */
} AdtActionNewUserT, *AdtActionNewUserTP;

typedef struct AdtActionNewGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;        /* Private data */
    PSTR dn;             /* DN/RDN of the parent */
    PSTR name;           /* Name */
    PSTR desc;           /* Description */
    PSTR namePreWin2000; /* Pre Windows-2000 name (samAccountName) */
    PSTR scope;           /* domain-local, global, universal */
} AdtActionNewGroupT, *AdtActionNewGroupTP;

typedef struct AdtActionNewComputer {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque; /* Private data */
    PSTR dn;      /* DN/RDN of the parent */
    PSTR name;    /* Name */
    PSTR desc;    /* Description */
    PSTR namePreWin2000; /* Pre Windows-2000 name (samAccountName) */
} AdtActionNewComputerT, *AdtActionNewComputerTP;

typedef struct AdtActionResetUserPassword {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR name;                /* DN/RDN, UPN, or SamAccountName */
    PSTR password;            /* Password */
    INT isNoMustChangePasswd; /* If set user do not need to change password at next logon */
    INT isNoCanChangePasswd;  /* User is not allowed to change password if set */
    INT isNoPasswdExpires;    /* Password never expires if set */
} AdtActionResetUserPasswordT, *ResetUserPasswordTP;

typedef struct AdtActionEnableUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR name;                /* DN/RDN, UPN, or SamAccountName */
} AdtActionEnableUserT, *EnableUserTP;

typedef struct AdtActionDisableUser {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR name;                /* DN/RDN, UPN, or SamAccountName */
} AdtActionDisableUserT, *DisableUserTP;

typedef struct AdtActionAddToGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR targetGroup;  /* Group to add user/group to (DN/RDN, CN) */
    PSTR user;         /* User to be added to the group (DN/RDN, UPN, or SamAccountName) */
    PSTR group;        /* Group to be added to the group (DN/RDN, or CN) */
} AdtActionAddToGroupT, *AdtActionAddToGroupTP;

typedef struct AdtActionRemoveFromGroup {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR targetGroup;  /* Group to remove user/group from (DN/RDN, CN) */
    PSTR user;         /* User to be removed from the group (DN/RDN, UPN, or SamAccountName) */
    PSTR group;        /* Group to be removed from the group (DN/RDN, or CN) */
} AdtActionRemoveFromGroupT, *AdtActionRemoveFromGroupTP;

typedef struct AdtActionUnlockAccount {
    AdtActionCode actionCode; /* Action code */
    VOID *opaque;             /* Private data */
    PSTR user;                /* User name (DN/RDN, UPN, or SamAccountName) */
    PSTR computer;            /* Computer name (DN/RDN, UPN, or SamAccountName) */
} AdtActionUnlockAccountT, *AdtActionUnlockAccountTP;

/**
 * Action definition. TODO: Add LW Open actions.
 */
typedef union AdtAction {
    AdtActionBaseT base;
    AdtActionNewCellT newCell;
    AdtActionEditCellT editCell;
    AdtActionEditCellUserT editCellUser;
    AdtActionEditCellGroupT editCellGroup;
    AdtActionAddToCellT addToCell;
    AdtActionRemoveFromCellT removeFromCell;
    AdtActionLinkCellT linkCell;
    AdtActionUnlinkCellT unlinkCell;
    AdtActionSearchCellT searchCells;
    AdtActionLookupCellT lookupCell;
    AdtActionLookupCellUserT lookupCellUser;
    AdtActionLookupCellGroupT lookupCellGroup;
    AdtActionDeleteCellT deleteCell;
    AdtActionDeleteObjectT deleteObject;
    AdtActionMoveObjectT moveObject;
   /* TODO: Add LW Open actions here */
    AdtActionSearchUserT searchUser;
    AdtActionSearchGroupT searchGroup;
    AdtActionSearchOuT searchOu;
    AdtActionSearchComputerT searchComputer;
    AdtActionSearchObjectT searchObject;
    AdtActionLookupObjectT lookupObject;
    AdtActionNewOuT newOu;
    AdtActionNewUserT newUser;
    AdtActionNewGroupT newGroup;
    AdtActionNewComputerT newComputer;
    AdtActionResetUserPasswordT resetUserPassword;
    AdtActionEnableUserT enableUser;
    AdtActionDisableUserT disableUser;
    AdtActionAddToGroupT addToGroup;
    AdtActionRemoveFromGroupT removeFromGroup;
    AdtActionUnlockAccountT unlockAccount;
} AdtActionT, *AdtActionTP, **AdtActionTPP;

typedef enum AdtOutputMode {
    AdtOutputModeStdout,    /* output goes directly to stdout and stderr */
    AdtOutputModeString     /* all output goes to strings AdtGetActionResult(), AdtGetActionStdout(), etc. */
} AdtOutputModeT;

/**
 * Action execution results. For now they are just placeholders but later
 * we can add action-specific fields.
 */
typedef struct AdtResultBase {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
} AdtResultBaseT, *AdtResultBaseTP;

typedef struct AdtResultNewCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultNewCellT, *AdtResultNewCellTP;

typedef struct AdtResultEditCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultEditCellT, *AdtResultEditCellTP;

typedef struct AdtResultEditCellUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultEditCellUserT, *AdtResultEditCellUserTP;

typedef struct AdtResultEditCellGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultEditCellGroupT, *AdtResultEditCellGroupTP;

typedef struct AdtResultDeleteCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultDeleteCellT, *AdtResultDeleteCellTP;

typedef struct AdtResultAddToCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultAddToCellT, *AdtResultAddToCellTP;

typedef struct AdtResultRemoveFromCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultRemoveFromCellT, *AdtResultRemoveFromCellTP;

typedef struct AdtResultLinkCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultLinkCellT, *AdtResultLinkCellTP;

typedef struct AdtResultUnlinkCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultUnlinkCellT, *AdtResultUnlinkCellTP;

typedef struct AdtResultSearchCells {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchCellsT, *AdtResultSearchCellsTP;

typedef struct AdtResultLookupCell {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultLookupCellT, *AdtResultLookupCellTP;

typedef struct AdtResultLookupCellUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultLookupCellUserT, *AdtResultLookupCellUserTP;

typedef struct AdtResultLookupCellGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultLookupCellGroupT, *AdtResultLookupCellGroupTP;

typedef struct AdtResultDeleteObject {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultDeleteObjectT, *AdtResultDeleteObjectTP;

typedef struct AdtResultMoveObject {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultMoveObjectT, *AdtResultMoveObjectTP;

typedef struct AdtResultSearchUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchUserT, *AdtResultSearchUserTP;

typedef struct AdtResultSearchGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchGroupT, *AdtResultSearchGroupTP;

typedef struct AdtResultSearchOu {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchOuT, *AdtResultSearchOuTP;

typedef struct AdtResultSearchComputer {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchComputerT, *AdtResultSearchComputerTP;

typedef struct AdtResultSearchObject {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultSearchObjectT, *AdtResultSearchObjectTP;

typedef struct AdtResultLookupObject {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultLookupObjectT, *AdtResultLookupObjectTP;

typedef struct AdtResultNewOu {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultNewOuT, *AdtResultNewOuTP;

typedef struct AdtResultNewUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultNewUserT, *AdtResultNewUserTP;

typedef struct AdtResultNewGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultNewGroupT, *AdtResultNewGroupTP;

typedef struct AdtResultNewComputer {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultNewComputerT, *AdtResultNewComputerTP;

typedef struct AdtResultResetUserPassword {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultResetUserPasswordT, *AdtResultResetUserPasswordTP;

typedef struct AdtResultEnableUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultEnableUserT, *AdtResultEnableUserTP;

typedef struct AdtResultDisableUser {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultDisableUserT, *AdtResultDisableUserTP;

typedef struct AdtResultAddToGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultAddToGroupT, *AdtResultAddToGroupTP;

typedef struct AdtResultRemoveFromGroup {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultRemoveFromGroupT, *AdtResultRemoveFromGroupTP;

typedef struct AdtResultUnlockAccount {
    AdtActionCode actionCode;  /* Action code */
    DWORD returnCode;          /* Execution result */
    PSTR resultStr;            /* Unstructured data */
    /*** Action specific structured data ***/
} AdtResultUnlockAccountT, *AdtResultUnlockAccountTP;

/**
 * Action execution result definition.
 */
typedef union AdtResult {
    AdtResultBaseT base;
    AdtResultNewCellT newCell;
    AdtResultEditCellT editCell;
    AdtResultEditCellUserT editCellUser;
    AdtResultEditCellGroupT editCellGroup;
    AdtResultAddToCellT addToCell;
    AdtResultRemoveFromCellT removeFromCell;
    AdtResultLinkCellT linkCell;
    AdtResultUnlinkCellT unlinkCell;
    AdtResultSearchCellsT searchCells;
    AdtResultLookupCellT lookupCell;
    AdtResultLookupCellUserT lookupCellUser;
    AdtResultLookupCellGroupT lookupCellGroup;
    AdtResultDeleteObjectT deleteObject;
    AdtResultDeleteCellT deleteCell;
    AdtResultMoveObjectT moveObject;
    /* TODO: Add LW Open actions here */
    AdtResultSearchUserT searchUser;
    AdtResultSearchGroupT searchGroup;
    AdtResultSearchOuT searchOu;
    AdtResultSearchComputerT searchComputer;
    AdtResultSearchObjectT searchObject;
    AdtResultLookupObjectT lookupObject;
    AdtResultNewOuT newOu;
    AdtActionNewUserT newUser;
    AdtActionNewGroupT newGroup;
    AdtActionNewComputerT newComputer;
    AdtResultResetUserPasswordT resetUserPassword;
    AdtResultEnableUserT enableUser;
    AdtResultDisableUserT disableUser;
    AdtResultAddToGroupT addToGroup;
    AdtResultRemoveFromGroupT removeFromGroup;
    AdtResultUnlockAccountT unlockAccount;
} AdtResultT, *AdtResultTP, **AdtResultTPP;

/********************************************************************/
/*                      AdTool API begin                            */
/********************************************************************/

/**
 * Initialize ADTool library. Call this method before using any other
 * AdTool API methods. In multi-threaded environment each thread must
 * have its context, so this method must be called from each thread.
 *
 * @param context Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD AdtOpen(OUT HANDLE *context);

/**
 * Close ADTool library. This method frees internally allocated memory.
 *
 * @param context Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD AdtClose(OUT HANDLE context);

/**
 * Set output mode of the adtool library.
 *
 * @param context Context reference returned by AdtOpen().
 * @param mode Output mode.
 */
VOID AdtSetOutputMode(IN HANDLE context, IN AdtOutputModeT mode);

/**
 * Create a new action object from command line arguments.
 *
 * @param context Context reference returned by AdtOpen().
 * @param argc Number of arguments.
 * @param argv Array of sting arguments.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgV(IN HANDLE context, IN INT argc, IN PCSTR *argv, OUT AdtActionTPP action);

/**
 * Create a new action from a command line string.
 * This string a normal lw-adtool command line without the
 * command component (lw-adtool), e.g. "-a new-cell -dn Enterprise"
 *
 * @param context Context reference returned by AdtOpen().
 * @param command Command line string.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgS(IN HANDLE context, IN PSTR command, OUT AdtActionTPP action);

/**
 * Create a new action from a command line string, using an existing
 * authenticated connection to an Active Directory server.
 * This string a normal lw-adtool command line without the
 * command component (lw-adtool), e.g. "-a new-cell -dn Enterprise"
 *
 * @param context Context reference returned by AdtOpen().
 * @param command Command line string.
 * @param serverConn Active Directory server connection.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgSC(IN HANDLE context, IN PSTR command, IN HANDLE serverConn, OUT AdtActionTPP action);

/**
 * Initialize action. Among other things, this method set references to Validate(),
 * CleanUp(), and Execute() functions. In order to use this method, you have to
 * allocate action memory first and set action fields as needed.
 * AdtGetAction() will finish initialization and return a ready to use action
 * object. When the action object is no longer needed, call AdtDisposeAction()
 * and then free the previously set fields and action memory.
 *
 * @param context Context reference returned by AdtOpen().
 * @param prePopulatedAction prepopulated action.
 * @param serverConn Authenticated Active Directory server connection.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetAction(IN HANDLE context, IN OUT AdtActionTP prePopulatedAction, IN HANDLE serverConn);

/**
 * Excute action.
 *
 * @param action Action to execute.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtExecuteAction(IN AdtActionTP action);

/**
 * Get result of action execution. The result is action dependent
 * and must be cast to an action-specific type or the base type.
 *
 * @param action Executed action.
 * @param execResult Execution result.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetExecResult(IN AdtActionTP action, OUT AdtResultTPP execResult);

/**
 * Get full stdout output of action creation and execution.
 *
 * @param context Context reference returned by AdtOpen().
 * @param str Stdout string.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetStdoutStr(IN HANDLE context, OUT PCSTR *str);

/**
 * Get full stderr output of action creation and execution.
 *
 * @param context Context reference returned by AdtOpen().
 * @param str Stderr string.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetStderrStr(IN HANDLE context, OUT PCSTR *str);

/**
 * Free action memory.
 *
 * @param action Action to be disposed.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtDisposeAction(IN AdtActionTP action);

/**
 * This method checks whether this is an AdTool-specific error and
 * remaps it as needed. It delegate the call to LW error mapper if
 * the error code is not known to AdTool.
 *
 * @param err Error code.
 */
PCSTR AdtGetErrorMsg(IN DWORD err);

/**
 * Look up action name by code.
 *
 * @param code Action code.
 * @return Action name or NULL if the code is unknown.
 */
PCSTR AdtGetActionName(IN AdtActionCode code);

/**
 * Look up action code by name.
 *
 * @param s Action name.
 * @return Action code or AdtBaseAction if the name is not recognized.
 */
AdtActionCode AdtGetActionCode(IN PSTR name);

/********************************************************************/
/*                        AdTool API end                            */
/********************************************************************/

#endif /* _ADTOOL_TYPES_H_ */
