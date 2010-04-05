/*
 *  gpplistutils.h
 *  lwimacinterface
 *
 *  Created by Sriram Nambakam on 8/21/07.
 *  Copyright 2007 Likewise Software, Inc. All rights reserved.
 *
 */

#ifndef __GPPLISTUTILS_H__
#define __GPPLISTUTILS_H__

#include <Carbon/Carbon.h>

int
GPReadPropertyListFile(
    CFStringRef pszFilePath,
    CFPropertyListRef* ppList,
    CFPropertyListFormat format
    );

int
GPSavePropertyList(
    CFPropertyListRef pList,
    CFStringRef pszFilePath,
    CFPropertyListFormat format
    );

void
CheckFileExists(
    char *pszPath,
    Boolean *pFileExists
    );
#endif /* __GPPLISTUTILS_H__ */


