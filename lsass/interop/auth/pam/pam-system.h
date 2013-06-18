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
 *        pam-system.h
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module
 * 
 *        System headers & definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PAM_SYSTEM_H__
#define __PAM_SYSTEM_H__

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_PASSWORD

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#elif HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#include <pam/pam_modules.h>
#endif

/* PAM_CONV_AGAIN & PAM_INCOMPLETE are not defined in solaris */
#ifndef PAM_CONV_AGAIN
#define PAM_CONV_AGAIN PAM_CONV_ERR
#endif

#ifndef PAM_INCOMPLETE
#define PAM_INCOMPLETE PAM_CONV_ERR
#endif

#endif /* __PAM_SYSTEM_H__ */
