/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
