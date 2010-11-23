/*
 * Likewise Identity shim idmap plugin
 *
 * Copyright (C) Gerald (Jerry) Carter      2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Disable the current idmap.h and support the legacy version.
   This only works if the pre-compiled version of nicludes.h
   does not exist. */

#define _IDMAP_H_

#include "wbclient.h"
#include "samba.h"

/* Version "3" was used in Samba 3.0.23 - 3.0.24 
 * But not change in VERSION interface number :-( */

#define SMB_IDMAP_INTERFACE_VERSION 2

#define ID_EMPTY        0x00
#define ID_USERID       0x01
#define ID_GROUPID      0x02
#define ID_OTHER        0x04

#define ID_TYPEMASK     0x0f

struct idmap_methods {
	NTSTATUS(*init) (char *params);
	NTSTATUS(*allocate_id) (unid_t * id, int id_type);
	NTSTATUS(*get_sid_from_id) (DOM_SID * sid, unid_t id, int id_type);
	NTSTATUS(*get_id_from_sid) (unid_t * id, int *id_type,
				    const DOM_SID * sid);
	NTSTATUS(*set_mapping) (const DOM_SID * sid, unid_t id, int id_type);
	NTSTATUS(*close) (void);
	void (*status) (void);
};

NTSTATUS smb_register_idmap(int version, const char *name,
                            struct idmap_methods *methods);

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_init(char *params)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_allocate_id(unid_t * id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 ********************************************************************/

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_sid_from_id(DOM_SID * sid, unid_t id, int type)
{
	struct wbcDomainSid wbc_sid;
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

	memset(&wbc_sid, 0x0, sizeof(wbc_sid));
	
	switch (type) {
	case ID_USERID:
		wbc_status = wbcUidToSid(id.uid, &wbc_sid);		
		break;
	case ID_GROUPID:
		wbc_status = wbcGidToSid(id.gid, &wbc_sid);
		break;
	default:
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (WBC_ERROR_IS_OK(wbc_status)) {
		/* structure are identical so we can get away with a 
		   memcpy here instead of a true copy fn(). */
		memcpy(sid, &wbc_sid, sizeof(*sid));
	}	

	return (WBC_ERROR_IS_OK(wbc_status)) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_id_from_sid(unid_t * id, int *type, const DOM_SID * sid)
{
	struct wbcDomainSid wbc_sid;
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
	int query_type = (*type) & ID_TYPEMASK;

	/* structure are identical so we can get away with a 
	   memcpy here instead of a true copy fn(). */
	memcpy(&wbc_sid, sid, sizeof(wbc_sid));
	
	switch (query_type) {
	case ID_USERID:
		wbc_status = wbcSidToUid(&wbc_sid, &id->uid);		
		break;
	case ID_GROUPID:
		wbc_status = wbcSidToGid(&wbc_sid, &id->gid);
		break;
	default:
		return NT_STATUS_INVALID_PARAMETER;
	}

	return (WBC_ERROR_IS_OK(wbc_status)) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_set_mapping(const DOM_SID * sid, unid_t id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_close(void)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static void lwi_idmap_status(void)
{
	return;
}

/*********************************************************************
 ********************************************************************/

static struct idmap_methods lwi_compat_methods = {
	.init = lwi_idmap_init,
	.allocate_id = lwi_allocate_id,
	.get_sid_from_id = lwi_get_sid_from_id,
	.get_id_from_sid = lwi_get_id_from_sid,
	.set_mapping = lwi_set_mapping,
	.close = lwi_idmap_close,
	.status = lwi_idmap_status
};

NTSTATUS init_module(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION,
				  "lwicompat_v3", &lwi_compat_methods);
}
