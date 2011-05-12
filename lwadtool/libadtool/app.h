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
 *        app.h
 *
 * Abstract:
 *
 *        Application context definitions.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 23, 2010
 *
 */

#ifndef _ADTOOL_APP_H_
#define _ADTOOL_APP_H_

/**
 * Action UI names.
 */
#define ADT_UNKNOWN_ACT                 "unknown"

/**
 * Enterprise edition.
 */
#define ADT_NEW_CELL_ACT                "new-cell"
#define ADT_EDIT_CELL_ACT               "edit-cell"
#define ADT_EDIT_CELL_USER_ACT          "edit-cell-user"
#define ADT_EDIT_CELL_GROUP_ACT         "edit-cell-group"
#define ADT_ADD_TO_CELL_ACT             "add-to-cell"
#define ADT_REMOVE_FROM_CELL_ACT        "remove-from-cell"
#define ADT_LINK_CELL_ACT               "link-cell"
#define ADT_UNLINK_CELL_ACT             "unlink-cell"
#define ADT_SEARCH_CELLS_ACT             "search-cells"
#define ADT_LOOKUP_CELL_ACT             "lookup-cell"
#define ADT_LOOKUP_CELL_USER_ACT        "lookup-cell-user"
#define ADT_lOOKUP_CELL_GROUP_ACT       "lookup-cell-group"
#define ADT_DELETE_CELL_ACT             "delete-cell"

/**
 * Open edition.
 */
#define ADT_DELETE_OBJECT_ACT           "delete-object"
#define ADT_MOVE_OBJECT_ACT             "move-object"
#define ADT_NEW_USER_ACT                "new-user"
#define ADT_NEW_GROUP_ACT               "new-group"
#define ADT_NEW_OU_ACT                  "new-ou"
#define ADT_NEW_COMPUTER_ACT            "new-computer"
#define ADT_SEARCH_USER_ACT             "search-user"
#define ADT_SEARCH_GROUP_ACT            "search-group"
#define ADT_SEARCH_OU_ACT               "search-ou"
#define ADT_SEARCH_COMPUTER_ACT         "search-computer"
#define ADT_SEARCH_OBJECT_ACT           "search-object"
#define ADT_LOOKUP_OBJECT_ACT           "lookup-object"
#define ADT_ENABLE_USER_ACT             "enable-user"
#define ADT_ENABLE_COMPUTER_ACT         "enable-computer"
#define ADT_DISABLE_USER_ACT            "disable-user"
#define ADT_DISABLE_COMPUTER_ACT        "disable-computer"
#define ADT_RESET_USER_PASSWORD_ACT     "reset-user-password"
#define ADT_RESET_COMPUTER_PASSWORD_ACT "reset-computer-password"
#define ADT_ADD_TO_GROUP_ACT            "add-to-group"
#define ADT_REMOVE_FROM_GROUP_ACT       "remove-from-group"
#define ADT_UNLOCK_ACCOUNT_ACT          "unlock-account"

#define A_DESC_HEADER "Description: "
#define A_EXAMPLE_HEADER "\n\nExample: "
#define A_EXAMPLES_HEADER "EXAMPLES\n\n"
#define NL_STR "\n\n"
#define NL_STR2 "\n"

/**
 * Search scope possible values.
 */
#define ADT_SEARCH_SCOPE_BASE      "base"
#define ADT_SEARCH_SCOPE_ONE       "one-level"
#define ADT_SEARCH_SCOPE_SUB       "subtree"

/**
 * Log levels
 */
typedef enum
{
  LogLevelNone = 0,
  LogLevelError,
  LogLevelWarning,
  LogLevelInfo,
  LogLevelVerbose,
  LogLevelTrace
} LogLevelT;

/**
 * Help options.
 */
typedef struct HelpOptions {
  INT isPrintUsage;     /* print usage and exit */
  INT isHelp;           /* print help and exit */
  INT isPrintVersion;   /* print version and exit */
} HelpOptionsT, *HelpOptionsTP;

/**
 * Generic options.
 */
typedef struct GenericOptions {
  LogLevelT logLevel;   /* log level */
  INT isQuiet;          /* silent execution */
  INT isReadOnly;       /* do not modify AD data */
  INT isPrintDN;        /* do not print anything to stdout except for DN */
} GenOptionsT, *GenOptionsTP;

/**
 * Connection options.
 */
typedef struct ConnOptions {
  PSTR serverAddress; /* IP address of the server to connect to */
  PSTR domainName;    /* Domain to connect to */
  INT  port;          /* TCP port number */
  INT  isNonSchema;   /* Non-schema mode. Default: schema. */
} ConnOptionsT, *ConnOptionsTP;

/**
 * Authentication options.
 */
typedef struct AuthOptions {
  PSTR logonAs;       /* account name for authentication */
  PSTR password;      /* password */
  PSTR keytabFile;    /* keytab file path */
  PSTR ticketCache;   /* krb5 ticket cache file path */
  INT  isNonSec;      /* Turns off secure mode. Simple bind will be used */
} AuthOptionsT, *AuthOptionsTP;

typedef enum ActionExecState {
    ActionNotReady = 0,
    ActionReady,
    ActionInProgress,
    ActionComplete
} ActionExecStateT;

typedef struct ADServerConnection {
    HANDLE conn;                      /* AD server connection */
    PSTR serverName;                  /* DNS name of the server */
    PSTR serverAddress;               /* IP address of the server to connect to */
    PSTR domainName;                  /* Domain to connect to */
    INT  port;                        /* TCP port number */
    PSTR defaultNC;                   /* Default naming context */
    BOOL isGCReady;                   /* TRUE is the server is GC-ready. */
} ADServerConnectionT, *ADServerConnectionTP;

/**
 * Application context.
 */
typedef struct AppContext {
  ADServerConnectionTP workConn;    /* Current working connection */
  ADServerConnectionT  searchConn;  /* AD connection for search operations */
  ADServerConnectionT  modifyConn;  /* AD connection for modify operations */
  BOOL isCredsSet;                  /* TRUE if credentials has been processed */
  BOOL isDefaultCell;               /* TRUE if using the default cell */

  /* Action-related data */
  AdtActionT action;                 /* action */
  PCSTR actionName;                  /* action name */
  INT isActionInitExternally;        /* the action structure has been prepopulated; hence it must be freed externally */
  DWORD (*actionInitMethod)     (AdtActionTP); /* action's init method */
  DWORD (*actionValidateMethod) (AdtActionTP); /* action's validate method */
  DWORD (*actionExecuteMethod)  (AdtActionTP); /* action's execute method */
  DWORD (*actionCleanUpMethod)  (AdtActionTP); /* action's cleanup method */
  AdtOutputModeT outputMode;         /* output mode */
  ActionExecStateT actionExecState;  /* action initialization status */
  AdtResultT execResult;             /* action execution result */
  PSTR stdoutStr;                    /* execution stdout collector */
  PSTR stderrStr;                    /* execution stderr collector */

  /* Parsed options and arguments */
  HelpOptionsT hopts;                /* help options */
  GenOptionsT gopts;                 /* generic options */
  ConnOptionsT copts;                /* connection options */
  AuthOptionsT aopts;                /* authentication options */

  /* popt data */
  struct poptOption *optionsTable;   /* options only table */
  struct poptOption *actionsTable;   /* options only table */
  struct poptOption *fullTable;      /* full syntax table */
  poptContext optionsCon;           /* popt context */
  poptContext fullCon;              /* popt context */

  /* Misc stuff */
  PSTR oID;                         /* AD object SID or GUID */
  PSTR oName;                       /* AD object name */
  PSTR oUID;                        /* UID */
  PSTR oGID;                        /* GID */
  PSTR oHomeDir;                    /* Default home dir */
  PSTR oShell;                      /* Default shell */
  PSTR oCellDn;                     /* Cell DN */
  INT optCount;                     /* Options counter */
  NET_CREDS_HANDLE creds;           /* Thread's credentials */
  BOOL rawTime;                     /* Do not format timestamps */
} AppContextT, *AppContextTP, **AppContextTPP;

/**
 * Get string presentation of the log level.
 *
 * @param level Log level
 * @return String presentation of the log level or an empty string if the
 * level is none.
 */
extern PCSTR LogLevel2Str(IN LogLevelT level);

/**
 * Print a message to stdout.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
extern VOID PrintStdout(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...);

/**
 * Print a message to stderr.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
extern VOID PrintStderr(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...);

/**
 * Print a message to the action execution result string or stdout.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
extern VOID PrintResult(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...);

/**
 * Get adtool error message.
 *
 * @param dwError Error code.
 * @return Error message.
 */
extern PCSTR GetErrorMsg(IN DWORD dwError);

/**
 * Save krb5 error message in the global error string.
 *
 * @param ctx Krb5 context.
 * @param err Krb5 error code.
 */
extern VOID Krb5SetGlobalErrorString(IN krb5_context ctx, IN krb5_error_code err);

/**
 * Process authentication arguments.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD ProcessAuthArgs(IN AppContextTP appContext);

/**
 * Process connection arguments.
 *
 * @param appContext Application context reference.
 * @param ipAddr AD server IP address.
 * @param port AD server TCP/IP port.
 * @param domain Domain name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
ProcessConnectArgs(IN AppContextTP appContext, IN PSTR ipAddr, IN INT port, IN PSTR domain);

/**
 * Find locate AD server to connect to.
 *
 * @param domain Domain to look AD server in.
 * @param ipAddress AD server IP address.
 * @param dnsName AD server DNS name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
FindAdServer(IN PSTR domain, OUT PSTR *ipAddress, OUT PSTR *dnsName);

/**
 * Process k5 options.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD ProcessK5Creds(IN AppContextTP appContext);

/**
 * Process the password option.
 *
 * @param aopts Authentication options reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD ProcessPassword(IN AuthOptionsTP aopts);

/**
 * Process the password option for a new user.
 *
 * @param aopts Authentication options reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD ProcessADUserPassword(OUT PSTR *pwd);

/**
 * Create cell group. (Schema mode)
 *
 * @param action Action reference.
 * @param odn DN of the connection point for the group
 * @param oid SID of the AD group
 * @param ogid Group ID
 * @param oname Name of the group
 * @param groupDN DN of the target AD group
 * @return 0 - on success; error code on failure.
 */
extern DWORD AdtAddGroupToCellS(IN AdtActionTP action,
        IN PSTR odn,
        IN PSTR oid,
        IN PSTR ogid,
        IN PSTR oname,
        IN PSTR groupDN);

/**
 * Create cell group. (Non-schema mode)
 *
 * @param action Action reference.
 * @param odn Group DN
 * @param oid SID of the AD group
 * @param ogid Group ID
 * @param oname Name of the group
 * @return 0 - on success; error code on failure.
 */
extern DWORD AdtAddGroupToCellNS(IN AdtActionTP action,
        IN PSTR odn,
        IN PSTR oid,
        IN PSTR ogid,
        IN PSTR oname);

/**
 * Open second connection for multi-forest searches if the name starts with
 * domain name or it is a UPN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 * return 0 on success; error code on failure.
 */
extern DWORD OpenADSearchConnectionDomain(IN AdtActionTP action, IN OUT PSTR *name);

/**
 * Open second connection for multi-forest searches if the name
 * specified via DN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 * return 0 on success; error code on failure.
 */
extern DWORD OpenADSearchConnectionDN(IN AdtActionTP action, IN OUT PSTR *name);

/**
 * Switch to connection that matches user's or group's UPN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 */
extern VOID SwitchToMatchingConnection(IN AdtActionTP action, IN OUT PSTR *name);

/**
 * Start using search connection.
 *
 * @param action Action reference.
 */
extern VOID SwitchToSearchConnection(IN AdtActionTP action);

/**
 * Start using search connection.
 *
 * @param action Action reference.
 */
extern VOID SwitchToModifyConnection(IN AdtActionTP action);

/**
 * Switch AD connection.
 *
 * @param action Action reference.
 */
extern VOID SwitchConnection(IN AdtActionTP action);

/**
 * Check if we are in multi-forest mode.
 *
 * @param action Action reference.
 * @return TRUE if we are in two-forest mode; FALSE otherwise.
 */
extern BOOL IsMultiForestMode(IN AdtActionTP action);

/**
 * Get current NT time.
 * The resulting string is dynamically allocated. Must be freed.
 *
 * @return NT time.
 */
extern PSTR GetCurrentNtTime();

/**
 * Actions' initialization methods.
 */
extern DWORD InitBaseAction(IN AdtActionTP action);
extern DWORD InitBaseActionWithNetAPI(IN AdtActionTP action);
extern DWORD InitAdtNewCellAction(IN AdtActionTP action);
extern DWORD InitAdtEditCellAction(IN AdtActionTP action);
extern DWORD InitAdtEditCellUserAction(IN AdtActionTP action);
extern DWORD InitAdtEditCellGroupAction(IN AdtActionTP action);
extern DWORD InitAdtAddToCellAction(IN AdtActionTP action);
extern DWORD InitAdtRemoveFromCellAction(IN AdtActionTP action);
extern DWORD InitAdtLinkCellAction(IN AdtActionTP action);
extern DWORD InitAdtUnlinkCellAction(IN AdtActionTP action);
extern DWORD InitAdtSearchCellsAction(IN AdtActionTP action);
extern DWORD InitAdtLookupCellAction(IN AdtActionTP action);
extern DWORD InitAdtLookupCellUserAction(IN AdtActionTP action);
extern DWORD InitAdtLookupCellGroupAction(IN AdtActionTP action);
extern DWORD InitAdtDeleteCellAction(IN AdtActionTP action);
extern DWORD InitAdtDeleteObjectAction(IN AdtActionTP action);
extern DWORD InitAdtMoveObjectAction(IN AdtActionTP action);
extern DWORD InitAdtNewUserAction(IN AdtActionTP action);
extern DWORD InitAdtNewGroupAction(IN AdtActionTP action);
extern DWORD InitAdtNewOuAction(IN AdtActionTP action);
extern DWORD InitAdtNewComputerAction(IN AdtActionTP action);
extern DWORD InitAdtSearchUserAction(IN AdtActionTP action);
extern DWORD InitAdtSearchGroupAction(IN AdtActionTP action);
extern DWORD InitAdtSearchOuAction(IN AdtActionTP action);
extern DWORD InitAdtSearchComputerAction(IN AdtActionTP action);
extern DWORD InitAdtSearchObjectAction(IN AdtActionTP action);
extern DWORD InitAdtLookupObjectAction(IN AdtActionTP action);
extern DWORD InitAdtEnableUserAction(IN AdtActionTP action);
extern DWORD InitAdtEnableComputerAction(IN AdtActionTP action);
extern DWORD InitAdtDisableUserAction(IN AdtActionTP action);
extern DWORD InitAdtDisableComputerAction(IN AdtActionTP action);
extern DWORD InitAdtResetUserPasswordAction(IN AdtActionTP action);
extern DWORD InitAdtResetComputerPasswordAction(IN AdtActionTP action);
extern DWORD InitAdtAddToGroupAction(IN AdtActionTP action);
extern DWORD InitAdtRemoveFromGroupAction(IN AdtActionTP action);
extern DWORD InitAdtUnlockAccountAction(IN AdtActionTP action);

/**
 * Actions' validate methods.
 */
extern DWORD ValidateBaseAction(IN AdtActionTP action);
extern DWORD ValidateAdtNewCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtEditCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtEditCellUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtEditCellGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtAddToCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtRemoveFromCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtLinkCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtUnlinkCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchCellsAction(IN AdtActionTP action);
extern DWORD ValidateAdtLookupCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtLookupCellUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtLookupCellGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtDeleteCellAction(IN AdtActionTP action);
extern DWORD ValidateAdtDeleteObjectAction(IN AdtActionTP action);
extern DWORD ValidateAdtMoveObjectAction(IN AdtActionTP action);
extern DWORD ValidateAdtNewUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtNewGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtNewOuAction(IN AdtActionTP action);
extern DWORD ValidateAdtNewComputerAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchOuAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchComputerAction(IN AdtActionTP action);
extern DWORD ValidateAdtSearchObjectAction(IN AdtActionTP action);
extern DWORD ValidateAdtLookupObjectAction(IN AdtActionTP action);
extern DWORD ValidateAdtEnableUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtEnableComputerAction(IN AdtActionTP action);
extern DWORD ValidateAdtDisableUserAction(IN AdtActionTP action);
extern DWORD ValidateAdtDisableComputerAction(IN AdtActionTP action);
extern DWORD ValidateAdtResetUserPasswordAction(IN AdtActionTP action);
extern DWORD ValidateAdtResetComputerPasswordAction(IN AdtActionTP action);
extern DWORD ValidateAdtAddToGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtRemoveFromGroupAction(IN AdtActionTP action);
extern DWORD ValidateAdtUnlockAccountAction(IN AdtActionTP action);

/**
 * Actions' execute method.
 */
extern DWORD ExecuteBaseAction(IN AdtActionTP action);
extern DWORD ExecuteAdtNewCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtEditCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtEditCellUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtEditCellGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtAddToCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtRemoveFromCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtLinkCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtUnlinkCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchCellsAction(IN AdtActionTP action);
extern DWORD ExecuteAdtLookupCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtLookupCellUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtLookupCellGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtDeleteCellAction(IN AdtActionTP action);
extern DWORD ExecuteAdtDeleteObjectAction(IN AdtActionTP action);
extern DWORD ExecuteAdtMoveObjectAction(IN AdtActionTP action);
extern DWORD ExecuteAdtNewUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtNewGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtNewOuAction(IN AdtActionTP action);
extern DWORD ExecuteAdtNewComputerAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchOuAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchComputerAction(IN AdtActionTP action);
extern DWORD ExecuteAdtSearchObjectAction(IN AdtActionTP action);
extern DWORD ExecuteAdtLookupObjectAction(IN AdtActionTP action);
extern DWORD ExecuteAdtEnableUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtEnableComputerAction(IN AdtActionTP action);
extern DWORD ExecuteAdtDisableUserAction(IN AdtActionTP action);
extern DWORD ExecuteAdtDisableComputerAction(IN AdtActionTP action);
extern DWORD ExecuteAdtResetUserPasswordAction(IN AdtActionTP action);
extern DWORD ExecuteAdtResetComputerPasswordAction(IN AdtActionTP action);
extern DWORD ExecuteAdtAddToGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtRemoveFromGroupAction(IN AdtActionTP action);
extern DWORD ExecuteAdtUnlockAccountAction(IN AdtActionTP action);

/**
 * Actions' clean up methods.
 */
extern DWORD CleanUpBaseAction(IN AdtActionTP action);
extern DWORD CleanUpBaseCellAction(IN AdtActionTP action);
extern DWORD CleanUpBaseWithNetAPIAction(IN AdtActionTP action);
extern DWORD CleanUpAdtNewCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtEditCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtEditCellUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtEditCellGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtAddToCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtRemoveFromCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtLinkCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtUnlinkCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchCellsAction(IN AdtActionTP action);
extern DWORD CleanUpAdtLookupCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtLookupCellUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtLookupCellGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtDeleteCellAction(IN AdtActionTP action);
extern DWORD CleanUpAdtDeleteObjectAction(IN AdtActionTP action);
extern DWORD CleanUpAdtMoveObjectAction(IN AdtActionTP action);
extern DWORD CleanUpAdtNewUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtNewGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtNewOuAction(IN AdtActionTP action);
extern DWORD CleanUpAdtNewComputerAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchOuAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchComputerAction(IN AdtActionTP action);
extern DWORD CleanUpAdtSearchObjectAction(IN AdtActionTP action);
extern DWORD CleanUpAdtLookupObjectAction(IN AdtActionTP action);
extern DWORD CleanUpAdtEnableUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtEnableComputerAction(IN AdtActionTP action);
extern DWORD CleanUpAdtDisableUserAction(IN AdtActionTP action);
extern DWORD CleanUpAdtDisableComputerAction(IN AdtActionTP action);
extern DWORD CleanUpAdtResetUserPasswordAction(IN AdtActionTP action);
extern DWORD CleanUpAdtResetComputerPasswordAction(IN AdtActionTP action);
extern DWORD CleanUpAdtAddToGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtRemoveFromGroupAction(IN AdtActionTP action);
extern DWORD CleanUpAdtUnlockAccountAction(IN AdtActionTP action);

/**
 * Error message buffer.
 */
extern CHAR adtLastErrMsg[];

#endif /* _ADTOOL_APP_H_ */
