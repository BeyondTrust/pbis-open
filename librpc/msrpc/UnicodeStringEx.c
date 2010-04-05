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

#ifdef __GNUC__
#include <string.h>
#include <iconv.h>
#endif

#include <lwrpc/types.h>
#include <lwrpc/allocate.h>
#include <wc16str.h>
#include <lw/ntstatus.h>
#include <lwrpc/unicodestring.h>


NTSTATUS InitUnicodeStringEx(UnicodeStringEx *u, const wchar16_t *s)
{
	if (u == NULL || s == NULL) {
        return STATUS_INVALID_PARAMETER;
	}

	u->string = wc16sdup(s);
	if (u->string == NULL) {
		return STATUS_NO_MEMORY;
	}

	u->len  = (UINT16) wc16slen(u->string) * sizeof(wchar16_t);
	u->size = (UINT16) (wc16slen(u->string) + 1) * sizeof(wchar16_t);

	return STATUS_SUCCESS;
}

NTSTATUS InitEmptyUnicodeString(UnicodeString *u)
{
    if (u == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    u->string = NULL;
    u->len  = 0;
    u->size = 0;

    return STATUS_SUCCESS;
}

wchar16_t *GetFromUnicodeStringEx(UnicodeStringEx *u)
{
    wchar16_t *out = NULL;

    if (u == NULL) return NULL;

    out = (wchar16_t*) malloc(sizeof(wchar16_t) * (u->size/2));
    if (out == NULL) return NULL;

    if (u->string) {
        wc16sncpy(out, u->string, u->len/2);
        out[u->len/2] = 0;

    } else {
        memset((void*)out, 0, u->size/2);
    }

    return out;
}


NTSTATUS CopyUnicodeStringEx(UnicodeStringEx *out, UnicodeStringEx *in)
{
    if (out == NULL || in == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (in->string != NULL) {
        out->string = wc16sndup(in->string, in->len/2);
        if (out->string == NULL) return STATUS_NO_MEMORY;
    }

    out->size = in->size;
    out->len  = in->len;

    return STATUS_SUCCESS;
}


UnicodeStringEx* InitUnicodeStringExArray(wchar16_t *sa[], size_t count)
{
	NTSTATUS status;
	UnicodeStringEx *ua = NULL;
	unsigned int i;

	ua = (UnicodeStringEx*) malloc(sizeof(UnicodeStringEx) * count);
	if (ua == NULL) return ua;

	for (i = 0; i < count; i++) {
		wchar16_t *s = sa[i];
		
		status = InitUnicodeStringEx(&(ua[i]), s);
		if (status != 0) {
			/* memory allocation problem occured so rollback and
			   safely return NULL */
			FreeUnicodeStringExArray(ua, count);
			return NULL;
		}
	}

	return ua;
}


void FreeUnicodeStringEx(UnicodeStringEx *u)
{
	if (u == NULL) return;
	SAFE_FREE(u->string);
}


void FreeUnicodeStringExArray(UnicodeStringEx *ua, size_t count)
{
	unsigned int i;
	if (ua == NULL) return;

	/* free the allocated copies of strings */
	for (i = 0; i < count; i++) {
		FreeUnicodeStringEx(&ua[i]);
	}

	/* free the array itself */
	free(ua);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
