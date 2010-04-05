/*++
	Linux DNS client library implementation
	Copyright (C) 2006 Krishna Ganugapati

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

++*/

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

#endif /* __DNSRECORD_H__ */
