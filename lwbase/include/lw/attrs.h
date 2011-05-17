/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        attrs.h
 *
 * Abstract:
 *
 *        Type and function attributes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWBASE_ATTRS_H__
#define __LWBASE_ATTRS_H__

#define LW_OPTIONAL
#define LW_IN
#define LW_OUT

#ifdef __cplusplus
#define LW_BEGIN_EXTERN_C extern "C" {
#define LW_END_EXTERN_C }
#else
#define LW_BEGIN_EXTERN_C
#define LW_END_EXTERN_C
#endif

#ifdef __GNUC__
#define LW_UNUSED __attribute__((unused))
#else
#define LW_UNUSED
#endif

#ifndef LW_STRICT_NAMESPACE

#define OPTIONAL LW_OPTIONAL
#define IN LW_IN
#define OUT LW_OUT

#endif /* LW_STRICT_NAMESPACE */

#endif
