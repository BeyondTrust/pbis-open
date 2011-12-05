/*
 * Copyright (c) BeyondTrust Software.  All rights Reserved.
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
 * BEYONDTRUST SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH BEYONDTRUST SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY BEYONDTRUST SOFTWARE, PLEASE CONTACT BEYONDTRUST SOFTWARE AT
 * license@beyondtrust.com
 */

/*
 * Module Name:
 *
 *        pthread-private
 *
 * Abstract:
 *
 *        Definition of replacement pthread function (private header)
 *
 * Authors: Kyle Stemen <kstemen@beyondtrust.com>
 *
 */
#ifndef __LWMSG_PTHREAD_PRIVATE_H__
#define __LWMSG_PTHREAD_PRIVATE_H__

#include <pthread.h>

int
_pthread_mutexattr_settype(
	pthread_mutexattr_t *,
	int
	);

#endif
