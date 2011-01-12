// DiskInfo.cpp : CDiskInfo のインプリメンテーション
#include "stdafx.h"
#include "ServerInfo.h"
#include "DiskInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CDiskInfo


STDMETHODIMP CDiskInfo::GetFreeDiskSpace(const wchar_t *wszDrive, hyper *hypFreeBytes)
{
	// TODO: この位置にインプリメント用のコードを追加してください

	USES_CONVERSION;

	DWORD dwSectorsPerCluster;
	DWORD dwBytesPerSector;
	DWORD dwBytesPerCluster;
	DWORD dwNumberOfFreeClusters;
	DWORD dwTotalNumberOfClusters;

	// Win95 OSR2より前GetDiskFreeSpaceEx()を利用することができないため,ここでは
	// GetDiskFreeSpace()を使用する.
	GetDiskFreeSpace(W2T(wszDrive), &dwSectorsPerCluster,
		&dwBytesPerSector, &dwNumberOfFreeClusters,
		&dwTotalNumberOfClusters);

	// 1クラスタ当りのバイト数をDWORDに格納する
	dwBytesPerCluster = dwSectorsPerCluster * dwBytesPerSector;

	// ただし，ディスク上のバイト数は，注意深く処理する必要がある．
	*hypFreeBytes = UInt32x32To64(dwNumberOfFreeClusters,
		dwBytesPerCluster);

	return S_OK;
}
