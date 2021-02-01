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


/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* Maximum number of log file size in KB that is supported */
    DWORD dwMaxLogSize;
    /* Maximum number of records that can be hold*/
    DWORD dwMaxRecords;
    /* Remove the events older than*/
    DWORD dwMaxAge;
    /* Purge the records at the interval */
    DWORD dwPurgeInterval;
    /* Flag to prune database*/
    BOOLEAN bRemoveAsNeeded;

    /* Who is allowed to read events */
    PSTR pszAllowReadTo;
    /* Who is allowed to write events */
    PSTR pszAllowWriteTo;
    /* Who is allowed to delete events */
    PSTR pszAllowDeleteTo;
} EVTSERVERINFO, *PEVTSERVERINFO;
