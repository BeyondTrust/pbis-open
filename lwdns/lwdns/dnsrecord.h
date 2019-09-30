/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsrecord.h
 *
 * Abstract:
 *
 *        BeyondTrust Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __DNSRECORD_H__
#define __DNSRECORD_H__

DWORD
DNSCreateDeleteRecord(
	PCSTR pszHost,
	WORD  wClass,
	WORD  wType,
	PDNS_RR_RECORD * ppDNSRecord
	);

DWORD
DNSCreatePtrRecord(
    PCSTR pszName,
    WORD  wClass,
    PCSTR pszDest,
    PDNS_RR_RECORD * ppDNSRecord
    );

DWORD
DNSCreateARecord(
	PCSTR pszHost,
	WORD  wClass,
	WORD  wType,
	DWORD dwIP,
	PDNS_RR_RECORD * ppDNSRecord
	);

DWORD
DNSCreateTKeyRecord(
	PCSTR szKeyName,
	PBYTE pKeyData,
	WORD dwKeyLen,
	PDNS_RR_RECORD * ppDNSRecord
	);

DWORD
DNSCreateTSIGRecord(
	PCSTR pszKeyName,
	DWORD dwTimeSigned,
	WORD wFudge,
	WORD wOriginalID,
	PBYTE pMac,
	WORD wMacSize,
	PDNS_RR_RECORD * ppDNSRecord
	);

DWORD
DNSCreateQuestionRecord(
	PCSTR pszQName,
	WORD  wQType,
	WORD  wQClass,
	PDNS_QUESTION_RECORD * ppDNSQuestionRecord
	);

VOID
DNSFreeQuestionRecordList(
    PDNS_QUESTION_RECORD* ppRecordList,
    DWORD dwNumRecords
    );

VOID
DNSFreeQuestionRecord(
	PDNS_QUESTION_RECORD pRecord
	);

DWORD
DNSCreateZoneRecord(
	PCSTR pszZName,
	PDNS_ZONE_RECORD * ppDNSZoneRecord
	);

VOID
DNSFreeZoneRecordList(
    PDNS_ZONE_RECORD* ppRecordList,
    DWORD dwNumRecords
    );

VOID
DNSFreeZoneRecord(
	PDNS_ZONE_RECORD pDNSZoneRecord
	);

DWORD
DNSCreateNameInUseRecord(
	PCSTR pszName,
	PDNS_RR_RECORD * ppDNSRRRecord
	);

DWORD
DNSCreateNameNotInUseRecord(
	PCSTR            pszName,
	PDNS_RR_RECORD * ppDNSRRRecord
	);

DWORD
DNSCreateRRSetExistsVIRecord(
	PCSTR pszName,
	WORD  wType,
	PDNS_RR_RECORD * ppDNSRRRecord
	);

DWORD
DNSCreateRRSetExistsVDRecord(
	PCSTR pszName,
	WORD  wType,
	PDNS_RR_RECORD * ppDNSRRRecord
	);

DWORD
DNSCreateRRSetNotExistsRecord(
	PCSTR pszName,
	WORD  wType,
	PDNS_RR_RECORD * ppDNSRRRecord
	);

VOID
DNSFreeRecordList(
    PDNS_RR_RECORD* ppRecordList,
    DWORD           dwNumRecords
    );

VOID
DNSFreeRecord(
    PDNS_RR_RECORD pRecord
    );

#ifndef HAVE_HPUX_OS
DWORD
DNSCreateAAAARecord(
    PCSTR pszHost,
    WORD  wClass,
    WORD  wType,
    PSTR  pszIPV6,
    PDNS_RR_RECORD * ppDNSRecord
    );
#endif

#endif /* __DNSRECORD_H__ */
