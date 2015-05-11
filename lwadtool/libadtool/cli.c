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
 *        cli.c
 *
 * Abstract:
 *        Command-line processing methods.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Apr 14, 2010
 *
 */

#include "includes.h"


/**
 * Populate a popt option.
 *
 * @param longName Long option name.
 * @param shortName Short option name.
 * @param argInfor Argument type description.
 * @param arg Argument.
 * @param val Parsed value
 * @param descrip Help item.
 * @param argDescrip Default value.
 */
static void MakeOption (
    IN struct poptOption *opt,
    IN PCSTR longName,  /*!< may be NULL */
    IN CHAR shortName,         /*!< may be NUL */
    IN UINT argInfo,
    /*@shared@*/ /*@null@*/
    IN PVOID arg,             /*!< depends on argInfo */
    IN INT val,                /*!< 0 means don't return, just update flag */
    /*@observer@*/ /*@null@*/
    IN PCSTR descrip,   /*!< description for autohelp -- may be NULL */
    /*@observer@*/ /*@null@*/
    IN PCSTR argDescrip    /*!< argument description for autohelp */
) {
    opt->longName = longName;
    opt->shortName = shortName;
    opt->argInfo = argInfo;
    opt->arg = arg;
    opt->val = val;
    opt->descrip = descrip;
    opt->argDescrip = argDescrip;
}

/**
 * Allocate popt options array.
 *
 * @param howMany The number of slots.
 * @return Array of options or NULL on failure.
 */
static struct poptOption**
MakeOptions(IN INT howMany) {
    struct poptOption **opts = NULL;

    if(LwAllocateMemory(sizeof(struct poptOption *), OUT_PPVOID(&opts)) != 0) {
        return NULL;
    }

    if(LwAllocateMemory(howMany * sizeof(struct poptOption), OUT_PPVOID(opts)) != 0) {
        return NULL;
    }

    return opts;
}


/**
 * Make full syntax table.
 *
 * @param optTable Options table.
 * @param actTable Actions table.
 * @return Full syntax table reference; NULL on failure.
 */
static struct poptOption **
MakeFullTable(IN struct poptOption *optTable, IN struct poptOption *actTable) {
    INT i = 0;

    struct poptOption **fullTable = MakeOptions(3);

    if(!fullTable) {
        return NULL;
    }

    MakeOption(&((*fullTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               optTable,
               0,
               "Options",
               NULL);

    MakeOption(&((*fullTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               actTable,
               0,
               "Actions",
               NULL);

    MakeOption(ADT_TABLEEND(&((*fullTable)[i++])));

    return fullTable;
}

/**
 * Find popt subtable of a given action and clone.
 *
 * @param table Actions table.
 * @param code Action code.
 * @return options table reference.
 */
struct poptOption *
CloneActionSubtable(IN struct poptOption *table, IN AdtActionCode code) {
    struct poptOption *subtable = NULL;
    struct poptOption **clone;
    INT itemsCount;
    INT i, j;
    INT isFound = 0;

    if(table == NULL) {
        return NULL;
    }

    for(i = 0; table[i].arg != NULL; ++i) {
        subtable = (struct poptOption *) table[i].arg;
        for(j = 0; subtable[j].arg != NULL; ++j) {
            if(subtable[j].val == code) {
                subtable = (struct poptOption *) subtable[j].arg;
                isFound = 1;
                break;
            }
        }

        if(isFound) {
            break;
        }
    }

    if(isFound) {
        for(i = 0; subtable[i].arg; ++i);
        itemsCount = i + 1;

        clone = MakeOptions(itemsCount);

        for(i = 0; i < itemsCount; ++i) {

            MakeOption(&((*clone)[i]),
                       subtable[i].longName,
                       subtable[i].shortName,
                       subtable[i].argInfo,
                       subtable[i].arg,
                       subtable[i].val,
                       subtable[i].descrip,
                       subtable[i].argDescrip);
        }

        subtable = *clone;
        LW_SAFE_FREE_MEMORY(clone);

        return subtable;
    }

    return NULL;
}

/**
 * Free options table.
 * @param opt Options table reference.
 */
VOID FreeOptions(IN struct poptOption *opt) {
    INT i;

    if(opt == NULL) {
        return;
    }

    for(i = 0; opt[i].arg; ++i) {
        if((opt[i].argInfo & POPT_ARG_MASK) == POPT_ARG_INCLUDE_TABLE) {
            // fprintf(stdout, "Freeing %s : %s\n", opt[i].longName, opt[i].descrip);
            FreeOptions((struct poptOption *) opt[i].arg);
            opt[i].arg = NULL;
        }
    }

    LW_SAFE_FREE_MEMORY(opt);
}

/**
 * Make full options table.
 *
 * @param appContext Application context.
 * @param acts Actions table or NULL to include all actions.
 * @return 0 on success; error code otherwise.
 */
DWORD MakeFullArgsTable(IN AppContextTP appContext, IN struct poptOption *acts) {
    DWORD dwError = 0;
    /**
     * Help options.
     */
    int i = 0;
    struct poptOption **helpOptions = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(helpOptions);
    MakeOption(&((*helpOptions)[i++]),
               "usage",
               'u',
               POPT_ARG_NONE,
               &(appContext->hopts.isPrintUsage),
               0,
               "Display brief usage message",
               NULL);
    MakeOption(&((*helpOptions)[i++]),
               "help",
               '?',
               POPT_ARG_NONE,
               &(appContext->hopts.isHelp),
               0,
               "Show this message, help on all actions (-a), or help on a specific action (-a <ACTION>).",
               NULL);
    MakeOption(&((*helpOptions)[i++]),
               "version",
               'v',
               POPT_ARG_NONE,
               &(appContext->hopts.isPrintVersion),
               0,
               "Print program version and exit.",
               NULL);
    MakeOption(ADT_TABLEEND(&((*helpOptions)[i++])));

    /**
     * Generic options.
     */
    i = 0;
    struct poptOption **genericOptions = MakeOptions(5);
    ADT_BAIL_ON_ALLOC_FAILURE(genericOptions);
    MakeOption(&((*genericOptions)[i++]),
               "log-level",
               'l',
               POPT_ARG_INT,
               &(appContext->gopts.logLevel),
               0,
               "Acceptable values: 1 (error), 2(warning), 3(info), 4(verbose) 5 (trace) (Default: warning).",
               "LOG_LEVEL");
    MakeOption(&((*genericOptions)[i++]),
               "quiet",
               'q',
               POPT_ARG_NONE,
               &(appContext->gopts.isQuiet),
               0,
               "Suppress printing to stdout. Just set the return code. print-dn option makes an exception.",
               NULL);
    MakeOption(&((*genericOptions)[i++]),
               "print-dn",
               't',
               POPT_ARG_NONE,
               &(appContext->gopts.isPrintDN),
               0,
               "Print DNs of the objects to be looked up, modified or searched for.",
               NULL);
    MakeOption(&((*genericOptions)[i++]),
               "read-only",
               'r',
               POPT_ARG_NONE,
               &(appContext->gopts.isReadOnly),
               0,
               "Do not actually modify directory objects when executing actions.",
               NULL);
    MakeOption(ADT_TABLEEND(&((*genericOptions)[i++])));

    /**
     * Connection options.
     */
    i = 0;
    struct poptOption **connectionOptions = MakeOptions(5);
    ADT_BAIL_ON_ALLOC_FAILURE(connectionOptions);
    MakeOption(&((*connectionOptions)[i++]),
               "server",
               's',
               POPT_ARG_STRING,
               &(appContext->copts.serverAddress),
               0,
               "Active Directory server to connect to.",
               NULL);
    MakeOption(&((*connectionOptions)[i++]),
               "domain",
               'd',
               POPT_ARG_STRING,
               &(appContext->copts.domainName),
               0,
               "Domain to connect to.",
               NULL);
    MakeOption(&((*connectionOptions)[i++]),
               "port",
               'p',
               POPT_ARG_INT,
               &(appContext->copts.port),
               0,
               "TCP port number",
               NULL);
    MakeOption(&((*connectionOptions)[i++]),
               "non-schema",
               'm',
               POPT_ARG_NONE,
               &(appContext->copts.isNonSchema),
               0,
               "Turn off schema mode",
               NULL);
    MakeOption(ADT_TABLEEND(&((*connectionOptions)[i++])));

    /**
     * Authentication options.
     */
    i = 0;
    struct poptOption **authOptions = MakeOptions(6);
    ADT_BAIL_ON_ALLOC_FAILURE(authOptions);
    MakeOption(&((*authOptions)[i++]),
               "logon-as",
               'n',
               POPT_ARG_STRING,
               &(appContext->aopts.logonAs),
               0,
               "User name or UPN.",
               NULL);
    MakeOption(&((*authOptions)[i++]),
               "passwd",
               'x',
               POPT_ARG_STRING,
               &(appContext->aopts.password),
               0,
               "Password for authentication. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*authOptions)[i++]),
               "keytab",
               'k',
               POPT_ARG_STRING,
               &(appContext->aopts.keytabFile),
               0,
               "Full path of keytab file, e.g. /etc/krb5.keytab",
               NULL);
    MakeOption(&((*authOptions)[i++]),
               "krb5cc",
               'c',
               POPT_ARG_STRING,
               &(appContext->aopts.ticketCache),
               0,
               "Full path of krb5 ticket cache file, e.g. /tmp/krb5cc_foo@centeris.com",
               NULL);
    MakeOption(&((*authOptions)[i++]),
               "no-sec",
               'z',
               POPT_ARG_NONE,
               &(appContext->aopts.isNonSec),
               0,
               "Turns off secure authentication. Simple bind will be used. Use with caution!",
               NULL);
    MakeOption(ADT_TABLEEND(&((*authOptions)[i++])));

    /**
     * Action options.
     */
    i = 0;
    struct poptOption **actOptions = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(actOptions);
    MakeOption(&((*actOptions)[i++]),
               "action",
               'a',
               POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL,
               &(appContext->actionName),
               'a',
               "Action to execute. Type \'--help -a\' for a list of actions, or \'--help -a <ACTION>\' for information on a specific action.",
               "<ACTION>");
    MakeOption(ADT_TABLEEND(&((*actOptions)[i++])));

    /**
     * Just for decoration.
     */
    i = 0;
    struct poptOption **fakeOptionsO = MakeOptions(1);
    ADT_BAIL_ON_ALLOC_FAILURE(fakeOptionsO);
    MakeOption(ADT_TABLEEND(&((*fakeOptionsO)[i++])));

    i = 0;
    struct poptOption **fakeOptionsA = MakeOptions(1);
    ADT_BAIL_ON_ALLOC_FAILURE(fakeOptionsA);
    MakeOption(ADT_TABLEEND(&((*fakeOptionsA)[i++])));

    /**
     * Options table.
     */
    i = 0;
    struct poptOption **optionsTable = MakeOptions(7);
    ADT_BAIL_ON_ALLOC_FAILURE(optionsTable);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *helpOptions,
               0,
               "HELP OPTIONS",
               NULL);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *genericOptions,
               0,
               "COMMON OPTIONS",
               NULL);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *connectionOptions,
               0,
               "CONNECTION OPTIONS",
               NULL);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *authOptions,
               0,
               "AUTHENTICATION OPTIONS",
               NULL);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *actOptions,
               0,
               "ACTION",
               NULL);
    MakeOption(&((*optionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *fakeOptionsO,
               0,
               ADT_APP_HELP_ACTIONS_USAGE,
               NULL);
    MakeOption(ADT_TABLEEND(&((*optionsTable)[i++])));

    struct poptOption **actionsTable = NULL;

    if(!acts) {
    /**
     * Action.
     */
    i = 0;
    struct poptOption **deleteObjectAction = MakeOptions(3);
    ADT_BAIL_ON_ALLOC_FAILURE(deleteObjectAction);
    MakeOption(&((*deleteObjectAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.deleteObject.dn),
               0,
               "DN/RDN of the object to delete. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*deleteObjectAction)[i++]),
               "force",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.deleteObject.isDeleteMembers),
               0,
               "remove all children first. Default: fail if the object has any child nodes",
               NULL);
    MakeOption(ADT_TABLEEND(&((*deleteObjectAction)[i++])));

    /**
     * Action.
     */
    i = 0;
    struct poptOption **moveObjectAction = MakeOptions(3);
    ADT_BAIL_ON_ALLOC_FAILURE(moveObjectAction);
    MakeOption(&((*moveObjectAction)[i++]),
               "from",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.moveObject.from),
               0,
               "DN/RDN of the object to move/rename. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*moveObjectAction)[i++]),
               "to",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.moveObject.to),
               0,
               "DN/RDN of the new object. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*moveObjectAction)[i++])));

    /**
     * Action.
     */
    i = 0;
    struct poptOption **newUserAction = MakeOptions(12);
    ADT_BAIL_ON_ALLOC_FAILURE(newUserAction);
    MakeOption(&((*newUserAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.dn),
               0,
               "DN/RDN of the parent container/OU containing the user. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "cn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.cn),
               0,
               "Common name (CN) of the new user. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "logon-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.name),
               0,
               "Logon name of the new user.  (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "pre-win-2000-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.namePreWin2000),
               0,
               "Pre Windows-2000 logon name.",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "first-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.nameFirst),
               0,
               "First name of the new user.",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "last-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.nameLast),
               0,
               "Last name of the new user.",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "description",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.desc),
               0,
               "Description of the user.",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "password",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newUser.password),
               0,
               "User\'s password. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "no-must-change-password",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.newUser.isNoMustChangePasswd),
               0,
               "User is not required to change the password at next logon. If omitted - user must change password at next logon unless \"--no-password-expires\" option is specified.",
               NULL);
    /*
    MakeOption(&((*newUserAction)[i++]),
               "no-can-change-password",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.newUser.isNoCanChangePasswd),
               0,
               "User is not allowed to change the password",
               NULL);
    */
    MakeOption(&((*newUserAction)[i++]),
               "no-password-expires",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.newUser.isNoPasswdExpires),
               0,
               "The password never expires.",
               NULL);
    MakeOption(&((*newUserAction)[i++]),
               "account-enabled",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.newUser.isAccountEnabled),
               0,
               "User account will be enabled. By default the account is disabled on creation.",
               NULL);
    MakeOption(ADT_TABLEEND(&((*newUserAction)[i++])));

    i = 0;
    struct poptOption **newGroupAction = MakeOptions(6);
    ADT_BAIL_ON_ALLOC_FAILURE(newGroupAction);
    MakeOption(&((*newGroupAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newGroup.dn),
               0,
               "DN/RDN of the parent container/OU containing the group. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newGroupAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newGroup.name),
               0,
               "Name of the group. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*newGroupAction)[i++]),
               "pre-win-2000-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newGroup.namePreWin2000),
               0,
               "Pre Windows-2000 logon name.",
               NULL);
    MakeOption(&((*newGroupAction)[i++]),
               "type",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newGroup.scope),
               0,
               "Group type. Acceptable values: domain-local, global, universal. Default: global",
               NULL);
    MakeOption(&((*newGroupAction)[i++]),
               "description",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newGroup.desc),
               0,
               "Description of the group.",
               NULL);
    MakeOption(ADT_TABLEEND(&((*newGroupAction)[i++])));

    i = 0;
    struct poptOption **newOuAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(newOuAction);
    MakeOption(&((*newOuAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newOu.dn),
               0,
               "DN/RDN of the new OU or DN/RDN of the parent if \"--name\" is present. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*newOuAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newOu.name),
               0,
               "Name of the new organizational unit. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newOuAction)[i++]),
               "description",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newOu.desc),
               0,
               "Description of the organizational unit",
               NULL);
    MakeOption(ADT_TABLEEND(&((*newOuAction)[i++])));

    i = 0;
    struct poptOption **newComputerAction = MakeOptions(5);
    ADT_BAIL_ON_ALLOC_FAILURE(newComputerAction);
    MakeOption(&((*newComputerAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newComputer.dn),
               0,
               "DN/RDN of the parent container/OU containing the computer. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*newComputerAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newComputer.name),
               0,
               "Name of the new computer. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*newComputerAction)[i++]),
               "pre-win-2000-name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newComputer.namePreWin2000),
               0,
               "Pre Windows-2000 name.",
               NULL);
    MakeOption(&((*newComputerAction)[i++]),
               "description",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.newComputer.desc),
               0,
               "Description of the computer",
               NULL);
    MakeOption(ADT_TABLEEND(&((*newComputerAction)[i++])));

    i = 0;
    struct poptOption **searchUserAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(searchUserAction);
    MakeOption(&((*searchUserAction)[i++]),
               "search-base",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchUser.base),
               0,
               "DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)",
               NULL);
    MakeOption(&((*searchUserAction)[i++]),
               "scope",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchUser.scope),
               0,
               "Search scope. Acceptable values: base, one-level, subtree. Default: subtree",
               NULL);
    MakeOption(&((*searchUserAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchUser.name),
               0,
               "Name of the user (DN/RDN, UPN, or SamAccountName). Wildcards (*) accepted as part of the name. [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*searchUserAction)[i++])));

    i = 0;
    struct poptOption **searchGroupAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(searchGroupAction);
    MakeOption(&((*searchGroupAction)[i++]),
               "search-base",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchGroup.base),
               0,
               "DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)",
               NULL);
    MakeOption(&((*searchGroupAction)[i++]),
               "scope",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchGroup.scope),
               0,
               "Search scope. Acceptable values: base, one-level, subtree. Default: subtree",
               NULL);
    MakeOption(&((*searchGroupAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchGroup.name),
               0,
               "Name of the group (DN/RDN, UPN, or SamAccountName). Wildcards (*) accepted as part of the name. [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*searchGroupAction)[i++])));

    i = 0;
    struct poptOption **searchOuAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(searchOuAction);
    MakeOption(&((*searchOuAction)[i++]),
               "search-base",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchOu.base),
               0,
               "DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)",
               NULL);
    MakeOption(&((*searchOuAction)[i++]),
               "scope",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchOu.scope),
               0,
               "Search scope. Acceptable values: base, one-level, subtree. Default: subtree",
               NULL);
    MakeOption(&((*searchOuAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchOu.name),
               0,
               "Name of the OU (DN/RDN, or CN). Wildcards (*) accepted as part of the name. [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*searchOuAction)[i++])));

    i = 0;
    struct poptOption **searchComputerAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(searchComputerAction);
    MakeOption(&((*searchComputerAction)[i++]),
               "search-base",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchComputer.base),
               0,
               "DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)",
               NULL);
    MakeOption(&((*searchComputerAction)[i++]),
               "scope",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchComputer.scope),
               0,
               "Search scope. Acceptable values: base, one-level, subtree. Default: subtree",
               NULL);
    MakeOption(&((*searchComputerAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchComputer.name),
               0,
               "Name of the computer (DN/RDN, UPN, or SamAccountName). Wildcards (*) accepted as part of the name. [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*searchComputerAction)[i++])));

    i = 0;
    struct poptOption **searchObjectAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(searchObjectAction);
    MakeOption(&((*searchObjectAction)[i++]),
               "search-base",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchObject.base),
               0,
               "DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)",
               NULL);
    MakeOption(&((*searchObjectAction)[i++]),
               "scope",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchObject.scope),
               0,
               "Search scope. Acceptable values: base, one-level, subtree. Default: subtree",
               NULL);
    MakeOption(&((*searchObjectAction)[i++]),
               "filter",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.searchObject.filter),
               0,
               "LDAP search filter (RFC 2254). Return all entries if omitted (Default: (objectClass=top))",
               NULL);
    MakeOption(ADT_TABLEEND(&((*searchObjectAction)[i++])));

    i = 0;
    struct poptOption **lookupObjectAction = MakeOptions(5);
    ADT_BAIL_ON_ALLOC_FAILURE(lookupObjectAction);
    MakeOption(&((*lookupObjectAction)[i++]),
               "dn",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.lookupObject.dn),
               0,
               "DN/RDN of the object to look up. (use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(&((*lookupObjectAction)[i++]),
               "attr",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.lookupObject.attr),
               0,
               "Attribute to show values of",
               NULL);
    MakeOption(&((*lookupObjectAction)[i++]),
               "raw-time",
               '\0',
               POPT_ARG_NONE,
               &(appContext->rawTime),
               0,
               "Do not format timestamps (show raw time data)",
               NULL);
    MakeOption(&((*lookupObjectAction)[i++]),
               "all",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.lookupObject.isAll),
               0,
               "Show values of all object attributes",
               NULL);
    MakeOption(ADT_TABLEEND(&((*lookupObjectAction)[i++])));

    i = 0;
    struct poptOption **enableUserAction = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(enableUserAction);
    MakeOption(&((*enableUserAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.enableUser.name),
               0,
               "Name of the user (DN/RDN, UPN, or samAccountName; use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*enableUserAction)[i++])));

    /*
    i = 0;
    struct poptOption **enableComputerAction = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(enableComputerAction);
    MakeOption(&((*enableComputerAction)[i++]),
               "(Not supported)",
               '\0',
               POPT_ARG_NONE,
               0,
               0,
               "(N/A)",
               NULL);
    MakeOption(ADT_TABLEEND(&((*enableComputerAction)[i++])));
    */

    i = 0;
    struct poptOption **disableUserAction = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(disableUserAction);
    MakeOption(&((*disableUserAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.disableUser.name),
               0,
               "Name of the user (DN/RDN, UPN, or samAccountName; use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*disableUserAction)[i++])));

    /*
    i = 0;
    struct poptOption **disableComputerAction = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(disableComputerAction);
    MakeOption(&((*disableComputerAction)[i++]),
               "(Not supported)",
               '\0',
               POPT_ARG_NONE,
               0,
               0,
               "(N/A)",
               NULL);
    MakeOption(ADT_TABLEEND(&((*disableComputerAction)[i++])));
    */

    i = 0;
    struct poptOption **resetUserPasswordAction = MakeOptions(5);
    ADT_BAIL_ON_ALLOC_FAILURE(resetUserPasswordAction);
    MakeOption(&((*resetUserPasswordAction)[i++]),
               "name",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.resetUserPassword.name),
               0,
               "User to change password for. (DN/RDN, UPN, or SamAccountName; use \'-\' for stdin input)  [X]",
               NULL);
    MakeOption(&((*resetUserPasswordAction)[i++]),
               "password",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.resetUserPassword.password),
               0,
               "User\'s password. If omitted only the password\'s properties may be changed but not the password itself. (use \'-\' for stdin input)",
               NULL);
    MakeOption(&((*resetUserPasswordAction)[i++]),
               "no-must-change-password",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.resetUserPassword.isNoMustChangePasswd),
               0,
               "User is not required to change the password at next logon. If omitted - user must change password at next logon unless \"--no-password-expires\" option is specified.",
               NULL);
    /*
    MakeOption(&((*resetUserPasswordAction)[i++]),
               "no-can-change-password",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.resetUserPassword.isNoCanChangePasswd),
               0,
               "User is not allowed to change the password",
               NULL);
    */
    MakeOption(&((*resetUserPasswordAction)[i++]),
               "no-password-expires",
               '\0',
               POPT_ARG_NONE,
               &(appContext->action.resetUserPassword.isNoPasswdExpires),
               0,
               "The password never expires.",
               NULL);
    MakeOption(ADT_TABLEEND(&((*resetUserPasswordAction)[i++])));

    /*
    i = 0;
    struct poptOption **resetComputerPasswordAction = MakeOptions(2);
    ADT_BAIL_ON_ALLOC_FAILURE(resetComputerPasswordAction);
    MakeOption(&((*resetComputerPasswordAction)[i++]),
               "(Not supported)",
               '\0',
               POPT_ARG_NONE,
               0,
               0,
               "(N/A)",
               NULL);
    MakeOption(ADT_TABLEEND(&((*resetComputerPasswordAction)[i++])));
    */

    i = 0;
    struct poptOption **addToGroupAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(addToGroupAction);
    MakeOption(&((*addToGroupAction)[i++]),
               "user",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.addToGroup.user),
               0,
               "User to add to the group (DN/RDN, UPN, or SamAccountName; use \'-\' for stdin input). [X]",
               NULL);
    MakeOption(&((*addToGroupAction)[i++]),
               "group",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.addToGroup.group),
               0,
               "Group to add to the group (DN/RDN, or CN; use \'-\' for stdin input). [X]",
               NULL);
    MakeOption(&((*addToGroupAction)[i++]),
               "to-group",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.addToGroup.targetGroup),
               0,
               "Group to add user or group to (DN/RDN , or CN; use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*addToGroupAction)[i++])));

    i = 0;
    struct poptOption **removeFromGroupAction = MakeOptions(4);
    ADT_BAIL_ON_ALLOC_FAILURE(removeFromGroupAction);
    MakeOption(&((*removeFromGroupAction)[i++]),
               "user",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.removeFromGroup.user),
               0,
               "User to remove from the group (DN/RDN, UPN, or SamAccountName; use \'-\' for stdin input). [X]",
               NULL);
    MakeOption(&((*removeFromGroupAction)[i++]),
               "group",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.removeFromGroup.group),
               0,
               "Group to remove from the group (DN/RDN, or CN; use \'-\' for stdin input). [X]",
               NULL);
    MakeOption(&((*removeFromGroupAction)[i++]),
               "from-group",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.removeFromGroup.targetGroup),
               0,
               "Group to remove user or group from (DN/RDN , or CN; use \'-\' for stdin input) [X]",
               NULL);
    MakeOption(ADT_TABLEEND(&((*removeFromGroupAction)[i++])));

    i = 0;
    struct poptOption **unlockAccountAction = MakeOptions(3);
    ADT_BAIL_ON_ALLOC_FAILURE(unlockAccountAction);
    MakeOption(&((*unlockAccountAction)[i++]),
               "user",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.unlockAccount.user),
               0,
               "User name (DN/RDN, UPN, or SamAccountName; use \'-\' for stdin input).",
               NULL);
    MakeOption(&((*unlockAccountAction)[i++]),
               "computer",
               '\0',
               POPT_ARG_STRING,
               &(appContext->action.unlockAccount.computer),
               0,
               "Computer name (DN/RDN, SPN, or SamAccountName; use \'-\' for stdin input).",
               NULL);
    MakeOption(ADT_TABLEEND(&((*unlockAccountAction)[i++])));

    /**
     * Open Edition options.
     */
    i = 0;
    struct poptOption **openActionsTable = MakeOptions(19);
    ADT_BAIL_ON_ALLOC_FAILURE(openActionsTable);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *addToGroupAction,
               AdtAddToGroupAction,
               ADT_ADD_TO_GROUP_ACT " - add a domain user/group to a security group.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *deleteObjectAction,
               AdtDeleteObjectAction,
               ADT_DELETE_OBJECT_ACT " - delete an object.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *disableUserAction,
               AdtDisableUserAction,
               ADT_DISABLE_USER_ACT " - disable a user account in Active Directory.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *enableUserAction,
               AdtEnableUserAction,
               ADT_ENABLE_USER_ACT " - enable a user account in Active Directory.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *unlockAccountAction,
               AdtUnlockAccountAction,
               ADT_UNLOCK_ACCOUNT_ACT " - unlock user or computer account.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *lookupObjectAction,
               AdtLookupObjectAction,
               ADT_LOOKUP_OBJECT_ACT " - retrieve object attributes.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *moveObjectAction,
               AdtMoveObjectAction,
               ADT_MOVE_OBJECT_ACT " - move/rename an object.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *newComputerAction,
               AdtNewComputerAction,
               ADT_NEW_COMPUTER_ACT " - create a new computer object.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *newGroupAction,
               AdtNewGroupAction,
               ADT_NEW_GROUP_ACT " - create a new global security group.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *newOuAction,
               AdtNewOuAction,
               ADT_NEW_OU_ACT " - create a new organizational unit.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *newUserAction,
               AdtNewUserAction,
               ADT_NEW_USER_ACT " - create a new user account.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *removeFromGroupAction,
               AdtRemoveFromGroupAction,
               ADT_REMOVE_FROM_GROUP_ACT " - remove a user/group from a security group.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *resetUserPasswordAction,
               AdtResetUserPasswordAction,
               ADT_RESET_USER_PASSWORD_ACT " - reset user\'s password.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *searchComputerAction,
               AdtSearchComputerAction,
               ADT_SEARCH_COMPUTER_ACT " - search for computer objects, print DNs.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *searchGroupAction,
               AdtSearchGroupAction,
               ADT_SEARCH_GROUP_ACT " - search for group objects, print DNs.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *searchObjectAction,
               AdtSearchObjectAction,
               ADT_SEARCH_OBJECT_ACT " - search for any type of objects using LDAP filter.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *searchOuAction,
               AdtSearchOuAction,
               ADT_SEARCH_OU_ACT " - search for organizational units, print DNs",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'O',
               POPT_ARG_INCLUDE_TABLE,
               *searchUserAction,
               AdtSearchUserAction,
               ADT_SEARCH_USER_ACT " - search for users, print DNs.",
               NULL);
    /*
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'N',
               POPT_ARG_INCLUDE_TABLE,
               *disableComputerAction,
               AdtDisableComputerAction,
               ADT_DISABLE_COMPUTER_ACT " - disable computer account in Active Directory.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'N',
               POPT_ARG_INCLUDE_TABLE,
               *resetComputerPasswordAction,
               AdtResetComputerPasswordAction,
               ADT_RESET_COMPUTER_PASSWORD_ACT " - reset computer\'s password.",
               NULL);
    MakeOption(&((*openActionsTable)[i++]),
               NULL,
               'N',
               POPT_ARG_INCLUDE_TABLE,
               *enableComputerAction,
               AdtEnableComputerAction,
               ADT_ENABLE_COMPUTER_ACT " - enable computer account in Active Directory.",
               NULL);
    */
    MakeOption(ADT_TABLEEND(&((*openActionsTable)[i++])));

    /**
     * Actions table.
     */
    i = 0;
    actionsTable = MakeOptions(3);
    ADT_BAIL_ON_ALLOC_FAILURE(actionsTable);
    MakeOption(&((*actionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *openActionsTable,
               0,
               "PowerBroker Identity Services - Open Edition:\n---------------------",
               NULL);
    MakeOption(&((*actionsTable)[i++]),
               NULL,
               '\0',
               POPT_ARG_INCLUDE_TABLE,
               *fakeOptionsA,
               0,
               ADT_APP_HELP_ACTION_USAGE,
               NULL);
    MakeOption(ADT_TABLEEND(&((*actionsTable)[i++])));

        LW_SAFE_FREE_MEMORY(openActionsTable);
        LW_SAFE_FREE_MEMORY(deleteObjectAction);
        LW_SAFE_FREE_MEMORY(moveObjectAction);
        LW_SAFE_FREE_MEMORY(newUserAction);
        LW_SAFE_FREE_MEMORY(newGroupAction);
        LW_SAFE_FREE_MEMORY(newOuAction);
        LW_SAFE_FREE_MEMORY(newComputerAction);
        LW_SAFE_FREE_MEMORY(searchUserAction);
        LW_SAFE_FREE_MEMORY(searchGroupAction);
        LW_SAFE_FREE_MEMORY(searchOuAction);
        LW_SAFE_FREE_MEMORY(searchComputerAction);
        LW_SAFE_FREE_MEMORY(searchObjectAction);
        LW_SAFE_FREE_MEMORY(lookupObjectAction);
        LW_SAFE_FREE_MEMORY(enableUserAction);
        LW_SAFE_FREE_MEMORY(disableUserAction);
        LW_SAFE_FREE_MEMORY(resetUserPasswordAction);
        LW_SAFE_FREE_MEMORY(addToGroupAction);
        LW_SAFE_FREE_MEMORY(removeFromGroupAction);
        LW_SAFE_FREE_MEMORY(unlockAccountAction);
        /*
        LW_SAFE_FREE_MEMORY(enableComputerAction);
        LW_SAFE_FREE_MEMORY(disableComputerAction);
        LW_SAFE_FREE_MEMORY(resetComputerPasswordAction);
        */
    }

    /**
     * Full syntax table.
     */
    if(acts) {
        appContext->actionsTable = acts;
        appContext->fullTable = (*MakeFullTable(*optionsTable, acts));
    }
    else {
        appContext->actionsTable = *actionsTable;
        appContext->fullTable = (*MakeFullTable(*optionsTable, *actionsTable));
        LW_SAFE_FREE_MEMORY(actionsTable);
    }

    appContext->optionsTable = *optionsTable;

    LW_SAFE_FREE_MEMORY(helpOptions);
    LW_SAFE_FREE_MEMORY(genericOptions);
    LW_SAFE_FREE_MEMORY(connectionOptions);
    LW_SAFE_FREE_MEMORY(authOptions);
    LW_SAFE_FREE_MEMORY(actOptions);
    LW_SAFE_FREE_MEMORY(fakeOptionsO);
    LW_SAFE_FREE_MEMORY(fakeOptionsA);
    LW_SAFE_FREE_MEMORY(optionsTable);

    cleanup:
       return(dwError);

    error:
       goto cleanup;
}

/**
 * Print usage.
 *
 * @param optCon Options context.
 * @param error Error message.
 * @param addl Additional information.
 * @param optionsFullCon Options full context.
 */
VOID PrintUsage(IN poptContext optCon, IN PCHAR error, IN PCHAR addl, IN poptContext optionsFullCon) {
    poptSetOtherOptionHelp(optCon, ADT_APP_ALT_USAGE);

    if (error) {
        fprintf(stderr, "%s: %s \n", error, addl);
    }

    poptPrintHelp(optionsFullCon, stdout, 0);
    PrintExamples();
    //poptPrintUsage(optCon, stderr, 0);
}

/**
 * Print list of actions
 *
 * @param table Pointer to options table
 * @param argc Number of arguments
 * @param argv Arguments
 */
VOID
PrintActionsList(IN struct poptOption *table, IN INT argc, IN PCSTR *argv) {
    struct poptOption *subtable;
    INT i, j;

    if(table == NULL) {
        return;
    }

    fprintf(stdout, "\n  List of Actions\n\n");

    for(i = 0; table[i].arg != NULL; ++i) {
        if(i == 0) {
            fprintf(stdout, "\n  Generic Active Directory actions:\n");
            fprintf(stdout, "  --------------------------------\n\n");
        }
        if(i == 1) {
            fprintf(stdout, "\n  PowerBroker Cell management actions:\n");
            fprintf(stdout, "  --------------------------------\n\n");
            fprintf(stdout, "  * Available only in Enterprise edition\n\n");
        }
        subtable = (struct poptOption *) table[i].arg;
        for(j = 0; subtable[j].arg != NULL; ++j) {
            // fprintf(stdout, "  %s (%c)\n", subtable[j].descrip, subtable[j].shortName);
            fprintf(stdout, "  %s\n", subtable[j].descrip);
        }
    }

    fprintf(stdout, ADT_APP_HELP_ACTION_USAGE "\n");
}

/**
 * Print help on action
 *
 * @param table Pointer to options table
 * @param code Action to get help on
 * @param argc Number of arguments
 * @param argv Arguments
 */
VOID
PrintActionHelp(IN struct poptOption *table, IN AdtActionCode code, IN INT argc, IN PCSTR *argv) {
    struct poptOption *subtable = NULL;
    INT i, j = 0;
    INT isFound = 0;
    PSTR str;

    if(table == NULL) {
        return;
    }

    for(i = 0; table[i].arg != NULL; ++i) {
        subtable = (struct poptOption *) table[i].arg;
        for(j = 0; subtable[j].arg != NULL; ++j) {
            if(subtable[j].val == code) {
                isFound = 1;
                break;
            }
        }

        if(isFound) {
            break;
        }
    }

    if((table[i].arg != NULL) && (subtable[j].arg != NULL)) {
        poptContext tmpCon =
            poptGetContext(
              "printActionHelp", argc, argv,
              (struct poptOption*) subtable[j].arg, 0
            );

        LwAllocateStringPrintf(
            &str, ADT_ACTION_HELP,
            AdtGetActionName(code), subtable[j].descrip
        );

        poptSetOtherOptionHelp(tmpCon, str);
        poptPrintHelp(tmpCon, stdout, 0);
        poptFreeContext(tmpCon);
        LwFreeString(str);
    }
}

/**
 * Print examples of usage.
 */
VOID PrintExamples() {
    PSTR s = ""
            A_EXAMPLES_HEADER
            "Create OU in a root naming context:"
            NL_STR2
            "adtool -a new-ou --dn OU=TestOu"
            NL_STR
            "Create OU in DC=department,DC=company,DC=com:"
            NL_STR2
            "adtool -a new-ou --dn OU=TestOu,DC=department,DC=company,DC=com"
            NL_STR
            "Create PowerBroker Cell in OU TestOU setting the default login shell property to /bin/ksh:"
            NL_STR2
            "adtool -a new-ou --dn OU=TestOu --default-login-shell=/bin/ksh"
            NL_STR
            "Create a new account for user TestUser in OU=Users,OU=TestOu:"
            NL_STR2
            "adtool -a new-user --dn OU=Users,OU=TestOu --cn=TestUserCN --logon-name=TestUser --password=$PASSWD"
            NL_STR
            "Enable the user account:"
            NL_STR2
            "adtool -a enable-user --name=TestUser"
            NL_STR
            "Reset user\'s password reading the password from TestUser.pwd file:"
            NL_STR2
            "cat TestUser.pwd | adtool -a reset-user-password --name=TestUser --password=- --no-password-expires"
            NL_STR
            "Create a new group in OU=Groups,OU=TestOu:"
            NL_STR2
            "adtool -a new-group --dn OU=Groups,OU=TestOu --pre-win-2000-name=TestGrooup --name=TestGroup"
            NL_STR
            "Look up \"description\" attribute of an OU specified by name with a wildcard:"
            NL_STR2
            "adtool -a search-ou --name=\'*RootOu\' -t | adtool -a lookup-object --dn=- --attr=description"
            NL_STR
            "Look up \"unixHomeDirectory\" attribute of a user with samAccountName TestUser:"
            NL_STR2
            "adtool -a search-user --name TestUser -t | adtool -a lookup-object --dn=- --attr=unixHomeDirectory"
            NL_STR
            "Look up \"userAccountControl\" attribute of a user with CN TestUserCN:"
            NL_STR2
            "adtool -a search-user --name CN=TestUserCN -t | adtool -a lookup-object --dn=- --attr=userAccountControl"
            NL_STR
            "Look up all attributes of an AD object using filter-based search:"
            NL_STR2
            "adtool -a search-object --filter \'(&(objectClass=person)(displayName=TestUser))\' -t | adtool -a lookup-object"
            NL_STR
            "Add user TestUser to group TestGroup:"
            NL_STR2
            "adtool -a add-to-group --user TestUser --to-group=TestGroup"
            NL_STR
            "Add group TestGroup2 to group TestGroup:"
            NL_STR2
            "adtool -a add-to-group --group TestGroup2 --to-group=TestGroup"
            NL_STR
            "Remove user TestUser from group TestGroup:"
            NL_STR2
            "adtool -a remove-from-group --user TestUser --from-group=TestGroup"
            NL_STR
            "Rename AD object OU=OldName and move it to a new location:"
            NL_STR2
            "adtool -a move-object --from OU=OldName,DC=department,DC=company,DC=com --to OU=NewName,OU=TestOU,DC=department,DC=company,DC=com"
            NL_STR
            "Add group TestGroup to PowerBroker Cell in TestOU:"
            NL_STR2
            "adtool -a add-to-cell --dn OU=TestOU,DC=department,DC=company,DC=com --group=TestGroup"
            NL_STR
            "Remove user TestUser from PowerBroker Cell in TestOU:"
            NL_STR2
            "adtool -a remove-from-cell --dn OU=TestOU,DC=department,DC=company,DC=com --user=TestUser"
            NL_STR
            "Search for cells in a specific location:"
            NL_STR2
            "adtool -a search-cells --search-base OU=department,DC=country,DC=company,DC=com"
            NL_STR
            "Link cell in OU=TestOU1 to the default cell in DC=country:"
            NL_STR2
            "adtool -a link-cell --source-dn OU=TestOU1,DC=department,DC=company,DC=com --target-dn DC=country,DC=company,DC=com"
            NL_STR
            "Unink cell in OU=TestOU1 from the default cell in DC=country:"
            NL_STR2
            "adtool -a unlink-cell --source-dn OU=TestOU1,DC=department,DC=company,DC=com --target-dn DC=country,DC=company,DC=com"
            NL_STR
            "Change the default login shell property of PowerBroker Cell in TestOU:"
            NL_STR2
            "adtool -a edit-cell --dn OU=TestOU --default-login-shell=/bin/csh"
            NL_STR
            "Find cells linked to PowerBroker Cell in OU=TestOU,DC=department,DC=company,DC=com:"
            NL_STR2
            "adtool -a lookup-cell --dn OU=TestOU --linked-cells"
            NL_STR
            "Look up login shell property of user TestUser in cell created in TestOU:"
            NL_STR2
            "adtool -a lookup-cell-user --dn OU=TestOU --user TestUser --login-shell"
            NL_STR
            "Change login shell property of user TestUser in cell created in TestOU:"
            NL_STR2
            "adtool -a edit-cell-user --dn OU=TestOU --user TestUser --login-shell=/usr/bin/ksh"
            NL_STR
            "Delete a cell object and all its children if any (--force):"
            NL_STR2
            "adtool -a delete-object --dn OU=TestOU --force"
            NL_STR
            "Search for PowerBroker Cells in root naming context containing user TestUser:"
            NL_STR2
            "adtool -a search-cells --user TestUser"
            NL_STR
            "Create a new PowerBroker Cell in OU=department:"
            NL_STR2
            "adtool -a new-cell --dn OU=department,DC=country,DC=company,DC=com"
            NL_STR
            "Create default PowerBroker Cell (assuming root naming context is DC=country,DC=company,DC=com):"
            NL_STR2
            "adtool -a new-cell --dn DC=country,DC=company,DC=com"
            NL_STR
            "Delete the default PowerBroker Cell (assuming root naming context is DC=country,DC=company,DC=com):"
            NL_STR2
            "adtool -a delete-cell --dn DC=country,DC=company,DC=com --force"
            ;

    fprintf(stdout, "%s\n", s);
    fprintf(stdout, "%s\n", ADT_APP_HELP_ACTIONS_LIST);
}


