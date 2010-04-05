/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Collector server settings interface
 *
 */

#ifndef __SETTINGS_P_H__
#define __SETTINGS_P_H__

static
DWORD
CltrReadDword(
    PCWSTR pwszName,
    DWORD dwDefault,
    PDWORD pdwResult
    );

#endif /* __SETTINGS_P_H__ */
