/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        addbquery.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        Password Database Query Templates
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __DB_QUERY_H__
#define __DB_QUERY_H__

typedef enum {
    DomainSID = 0,            
    DomainName,               //1
    DomainDnsName,            //2
    HostName,                 //3
    HostDnsDomain,            //4
    MachineAccountName,       //5
    MachineAccountPassword,   //6
    PwdCreationTimestamp,     //7
    PwdClientModifyTimestamp, //8
    SchannelType              //9
} MachinePwdTableIndex;


#define DB_QUERY_CREATE_MACHINEPWD_TABLE                             \
    "CREATE TABLE machinepwd (DomainSID                varchar(512), \
                              DomainName               varchar(256), \
                              DomainDnsName            varchar(256), \
                              HostName                 varchar(256), \
                              MachineAccountName       varchar(256), \
                              MachineAccountPassword   varchar(256), \
                              PwdCreationTimestamp     INTEGER,      \
                              PwdClientModifyTimestamp INTEGER,      \
                              SchannelType             INTEGER,      \
                              HostDnsDomain            varchar(256)  \
                             )"


#define DB_QUERY_INSERT_MACHINEPWD_ENTRY                         \
    "INSERT INTO machinepwd                                      \
                 ( DomainSID,                                    \
                   DomainName,                                   \
                   DomainDnsName,                                \
                   HostName,                                     \
                   HostDnsDomain,                                \
                   MachineAccountName,                           \
                   MachineAccountPassword,                       \
                   PwdCreationTimestamp,                         \
                   PwdClientModifyTimestamp,                     \
                   SchannelType                                  \
                 ) VALUES                                        \
                 ( %Q,                                           \
                   upper(%Q),                                    \
                   upper(%Q),                                    \
                   upper(%Q),                                    \
                   %Q,                                           \
                   upper(%Q),                                    \
                   %Q,                                           \
                   %u,                                           \
                   %u,                                           \
                   %u                                            \
                 )"

#define DB_QUERY_GET_MACHINEPWD_BY_DOMAIN_DNS_NAME               \
    "SELECT DomainSID,                                           \
            upper(DomainName),                                   \
            upper(DomainDnsName),                                \
            upper(HostName),                                     \
            HostDnsDomain,                                       \
            upper(MachineAccountName),                           \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd                                           \
      WHERE upper(DomainDnsName) = upper(%Q)"

#define DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME                     \
    "SELECT DomainSID,                                           \
            upper(DomainName),                                   \
            upper(DomainDnsName),                                \
            upper(HostName),                                     \
            HostDnsDomain,                                       \
            upper(MachineAccountName),                           \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd                                           \
      WHERE upper(HostName) = upper(%Q)"

#define DB_QUERY_GET_MACHINEPWD_ENTRIES                          \
    "SELECT DomainSID,                                           \
            DomainName,                                          \
            DomainDnsName,                                       \
            HostName,                                            \
            HostDnsDomain,                                       \
            MachineAccountName,                                  \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd"

#define DB_QUERY_DELETE_MACHINEPWD_ENTRY_BY_HOST_NAME            \
    "DELETE                                                      \
       FROM machinepwd                                           \
      WHERE upper(HostName) = upper(%Q)"

#define DB_QUERY_DELETE_MACHINEPWD_ENTRY                         \
    "DELETE                                                      \
       FROM machinepwd                                           \
      WHERE DomainSID = %Q                                       \
        AND upper(MachineAccountName) = upper(%Q)"

#define DB_QUERY_DELETE_ALL_MACHINEPWD_ENTRIES                   \
    "DELETE from machinepwd"

#endif /* __DB_QUERY_H__ */

