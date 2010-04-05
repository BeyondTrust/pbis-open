/*
 *  gpplistutils.c
 *  lwimacinterface
 *
 *  Created by Sriram Nambakam on 8/21/07.
 *  Copyright 2007 Likewise Software, Inc. All rights reserved.
 *
 */

#include "gpplistutils.h"
#include "lwimaciface_errcodes.h"
#include <sys/stat.h>

#include <unistd.h>

int
GPReadPropertyListFile(
    CFStringRef pszFilePath,
    CFPropertyListRef* ppList,
    CFPropertyListFormat pListFormat
    )
{
    int result = 0;
    CFURLRef fileURL = NULL;
    CFReadStreamRef inStream = NULL;
    CFPropertyListRef pList = NULL;
    CFStringRef errorString = NULL;
    
	fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, 
                                             pszFilePath, 
                                             kCFURLPOSIXPathStyle, 0);
    if (fileURL == NULL) {
        result = GP_MAC_ITF_FAILED_OPEN_FILE;
        goto cleanup;
    }
    
    inStream = CFReadStreamCreateWithFile( kCFAllocatorDefault, 
                                           fileURL);
    if (fileURL == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_STREAM;
        goto cleanup;
    }
    
    if (!CFReadStreamOpen(inStream)) {
        result = GP_MAC_ITF_FAILED_OPEN_STREAM;
        goto cleanup;
    }
    
    pList = CFPropertyListCreateFromStream( kCFAllocatorDefault,
                                            inStream,
                                            0,
                                            kCFPropertyListMutableContainersAndLeaves,
                                            &pListFormat,
                                            &errorString);
    if (pList == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_PLIST;
        goto cleanup;
    }
    
    *ppList = pList;
    pList = NULL;

cleanup:

    if (pList) {
       CFRelease(pList);
    }
    
    if (inStream) {
       CFReadStreamClose(inStream);
    }
    
    if (fileURL) {
       CFRelease(fileURL);
    }
    
    if (errorString) {
       CFRelease(errorString);
    }
    
    return result;
}

int
GPSavePropertyList(
    CFPropertyListRef pList,
    CFStringRef pszFilePath,
    CFPropertyListFormat pListFormat
    )
{
    int result = 0;
    CFURLRef fileURL = NULL;
    CFWriteStreamRef outStream = NULL;
    CFStringRef errorString = NULL;
    CFStringRef pszTmpFilePath = NULL;
    int bRemoveFile = 0;
    char szFilePath[PATH_MAX+1];
    char szTmpFilePath[PATH_MAX+1];
    
    *szFilePath = '\0';
    *szTmpFilePath = '\0';

    pszTmpFilePath = CFStringCreateWithFormat( NULL, 
                                               NULL, 
                                               CFSTR("%@.lwidentity.tmp"), 
                                               pszFilePath);
    
    fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, 
                                             pszTmpFilePath, 
                                             kCFURLPOSIXPathStyle, 
                                             0);
    if (fileURL == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_FILE;
        goto cleanup;
    }
    
    bRemoveFile = 1;
    
    outStream = CFWriteStreamCreateWithFile( kCFAllocatorDefault, 
                                             fileURL);
    if (outStream == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_STREAM;
        goto cleanup;
    }
    
    if (!CFWriteStreamOpen(outStream)) {
        result = GP_MAC_ITF_FAILED_OPEN_STREAM;
        goto cleanup;
    }
    
    if (!CFPropertyListWriteToStream (pList, outStream, pListFormat, &errorString)) {
        result = GP_MAC_ITF_FAILED_WRITE_PLIST;
        goto cleanup;
    }
    
    CFWriteStreamClose(outStream);
    outStream = NULL;
	
	if (!CFStringGetCString(pszFilePath, szFilePath, PATH_MAX, kCFStringEncodingASCII) ||
	    !CFStringGetCString(pszTmpFilePath, szTmpFilePath, PATH_MAX, kCFStringEncodingASCII)) {
	    result = GP_MAC_ITF_FAILED_CONVERT_STRING_ENCODING;
	    goto cleanup;
	}
    
    if (rename(szTmpFilePath, szFilePath) < 0) {
        result = GP_MAC_ITF_FAILED_MOVE_FILE;
        goto cleanup;
    }
    
    bRemoveFile = 0;

cleanup:

    if (errorString) {
       CFRelease(errorString);
    }
    
    if (outStream) {
       CFWriteStreamClose(outStream);
    }
    
    if (fileURL) {
       CFRelease(fileURL);
    }
    
    if (bRemoveFile && *szTmpFilePath) {
       unlink(szTmpFilePath);
    }
    
    if (pszTmpFilePath) {
       CFRelease(pszTmpFilePath);
    }

    return result;
}

void
CheckFileExists(
    char *pszPath,
    Boolean *pbFileExists
    )
{
    struct stat statbuf;

    memset( &statbuf, 
            0, 
            sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ENOENT || errno == ENOTDIR) {
                *pbFileExists = 0;
                break;
            }
        } else {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0);
            break;
        }
    }
}

