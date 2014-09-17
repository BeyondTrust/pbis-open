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
 *        cli.h
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

#ifndef _ADTOOL_CLI_H_
#define _ADTOOL_CLI_H_

/**
 * Print usage.
 *
 * @param optCon Options context.
 * @param error Error message.
 * @param addl Additional information.
 * @param optFullCon Options full context.
 */
extern VOID PrintUsage(IN poptContext optCon, IN PCHAR error, IN PCHAR addl, IN poptContext optFullCon);

/**
 * Print list of actions
 *
 * @param table Pointer to options table
 * @param argc Number of arguments
 * @param argv Arguments
 */
extern VOID PrintActionsList(IN struct poptOption *table, IN INT argc, IN PCSTR *argv);

/**
 * Print help on action
 *
 * @param table Pointer to options table
 * @param code Action to get help on
 * @param argc Number of arguments
 * @param argv Arguments
 */
extern VOID PrintActionHelp(IN struct poptOption *table, IN AdtActionCode code, IN INT argc, IN PCSTR *argv);

/**
 * Print examples of usage.
 */
extern VOID PrintExamples();

/**
 * Find popt subtable of a given action and clone.
 *
 * @param table Actions table.
 * @param code Action code.
 * @return options table reference.
 */
extern struct poptOption *CloneActionSubtable(IN struct poptOption *table, IN AdtActionCode code);

/**
 * Free options table.
 * @param opt Options table reference.
 */
extern VOID FreeOptions(IN struct poptOption *opt);

/**
 * Make full options table.
 *
 * @param appContext Application context.
 * @param acts Actions table or NULL to include all actions.
 * @return 0 on success; error code otherwise.
 */
extern DWORD MakeFullArgsTable(IN AppContextTP appContext, IN struct poptOption *acts);

#endif /* _ADTOOL_CLI_H_ */
