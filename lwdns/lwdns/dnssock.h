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


DWORD
DNSTCPOpen(
	PCSTR    pszNameServer,
	PHANDLE  phDNSServer
	);



DWORD
DNSUDPOpen(
    PCSTR    pszNameServer,
	PHANDLE  phDNSServer
	);



DWORD
DNSTCPClose(HANDLE hBindServer);


DWORD
DNSUDPClose(HANDLE hBindServer);

DWORD
DNSClose(
    HANDLE hDNSServer
    );

DWORD
DNSSendTCPRequest(
	HANDLE hDNSHandle,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesSent
	);

DWORD
DNSSendUDPRequest(
	HANDLE hDNSHandle,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesSent
	);


DWORD
DNSTCPReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);

DWORD
DNSUDPReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);

DWORD
DNSReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);


DWORD
DNSCreateSendBuffer(
	HANDLE * phDNSSendBuffer
	);


DWORD
DNSMarshallBuffer(
	HANDLE hDNSSendBuffer,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesWritten
	);

void
DNSFreeSendBuffer(
	HANDLE hDNSSendBuffer
	);

DWORD
DNSSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);


DWORD
DNSTCPSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);


DWORD
DNSUDPSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);

DWORD
DNSDumpSendBufferContext(
	HANDLE hSendBuffer
	);

DWORD
DNSDumpRecvBufferContext(
	HANDLE hRecvBuffer
	);

DWORD
DNSCreateReceiveBuffer(
	HANDLE * phDNSRecvBuffer
	);

DWORD
DNSUnmarshallBuffer(
	HANDLE hDNSRecvBuffer,
	PBYTE pDNSRecvBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesRead
	);

DWORD
DNSUnmarshallDomainNameAtOffset(
	HANDLE hRecvBuffer,
	WORD wOffset,
	PDNS_DOMAIN_NAME * ppDomainName
	);

DWORD
DNSReceiveBufferMoveBackIndex(
	HANDLE hRecvBuffer,
	WORD wOffset
	);

VOID
DNSFreeSendBufferContext(
	HANDLE hSendBuffer
	);

VOID
DNSFreeReceiveBufferContext(
	HANDLE hRecvBuffer
	);

DWORD
DNSGetSendBufferContextSize(
	HANDLE hSendBuffer
	);


PBYTE
DNSGetSendBufferContextBuffer(
	HANDLE hSendBuffer
	);


