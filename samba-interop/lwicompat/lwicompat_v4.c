/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Likewise Identity shim idmap plugin
 *
 * Copyright (C) Gerald (Jerry) Carter
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

/* Version 4 was introduced in Samba 3.0.25 */

#define SMB_IDMAP_INTERFACE_VERSION 4

#define ID_EMPTY        0x00
#define ID_USERID       0x01
#define ID_GROUPID      0x02
#define ID_OTHER        0x04

struct idmap_domain {
	const char *name;
	bool default_domain;
	bool readonly;
	void *private_data;
	struct idmap_methods *methods;
};

enum id_mapping {
	ID_UNKNOWN = 0,
	ID_MAPPED,
	ID_UNMAPPED,
	ID_EXPIRED
};

enum id_type {
	ID_TYPE_NOT_SPECIFIED = 0,
	ID_TYPE_UID,
	ID_TYPE_GID
};

struct unixid {
	uint32_t id;
	enum id_type type;
};

struct id_map {
	DOM_SID *sid;
	struct unixid xid;
	enum id_mapping status;
};

/* Filled out by IDMAP backends */
struct idmap_methods {

	/* Called when backend is first loaded */
	NTSTATUS(*init) (struct idmap_domain * dom, const char *compat_params);

	NTSTATUS(*unixids_to_sids) (struct idmap_domain * dom,
				    struct id_map ** ids);
	NTSTATUS(*sids_to_unixids) (struct idmap_domain * dom,
				    struct id_map ** ids);
	NTSTATUS(*set_mapping) (struct idmap_domain * dom,
				const struct id_map * map);
	NTSTATUS(*remove_mapping) (struct idmap_domain * dom,
				   const struct id_map * map);

	/* Called to dump backends data */
	/* NOTE: caller must use talloc_free to free maps when done */
	NTSTATUS(*dump_data) (struct idmap_domain * dom, struct id_map ** maps,
			      int *num_maps);

	/* Called when backend is unloaded */
	NTSTATUS(*close_fn) (struct idmap_domain * dom);
};

NTSTATUS smb_register_idmap(int version, const char *name,
                            struct idmap_methods *methods);

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_init(struct idmap_domain *dom,
			       const char *compat_params)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/
static NTSTATUS lwi_get_sid_from_id(struct idmap_domain *dom,
				    struct id_map **ids)
{
	int i;
	NTSTATUS nt_status = NT_STATUS_NONE_MAPPED;	

	/* loop over the array and issue requests one at a time */

	for (i = 0; ids[i]; i++) {
		struct wbcDomainSid wbc_sid;
		wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

		ids[i]->status = ID_UNMAPPED;

		switch (ids[i]->xid.type) {
		case ID_TYPE_UID:
			wbc_status = wbcUidToSid(ids[i]->xid.id, &wbc_sid);			
			break;
		case ID_TYPE_GID:
			wbc_status = wbcGidToSid(ids[i]->xid.id, &wbc_sid);
			break;
		default:
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (WBC_ERROR_IS_OK(wbc_status)) {
			memcpy(ids[i]->sid, &wbc_sid, sizeof(*ids[i]->sid));
			ids[i]->status = ID_MAPPED;
			nt_status = NT_STATUS_OK;			
		}
	}

	return nt_status;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_id_from_sid(struct idmap_domain *dom,
				    struct id_map **ids)
{
	NTSTATUS nt_status = NT_STATUS_NONE_MAPPED;	
	int i;

	for (i = 0; ids[i]; i++) {
		struct wbcDomainSid wbc_sid;
		wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

		/* Setup */
		memcpy(&wbc_sid, ids[i]->sid, sizeof(wbc_sid));
		ids[i]->status = ID_UNMAPPED;

		switch (ids[i]->xid.type) {
		case ID_TYPE_UID:
			wbc_status = wbcSidToUid(&wbc_sid, (uid_t*)&ids[i]->xid.id);
			break;
		case ID_TYPE_GID:
			wbc_status = wbcSidToGid(&wbc_sid, (gid_t*)&ids[i]->xid.id);
			break;
		default:
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (WBC_ERROR_IS_OK(wbc_status)) {
			ids[i]->status = ID_MAPPED;
			nt_status = NT_STATUS_OK;
		}
	}
	
	return nt_status;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_set_mapping(struct idmap_domain *dom,
				const struct id_map *map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_remove_mapping(struct idmap_domain *dom,
				   const struct id_map *map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_dump(struct idmap_domain *dom,
			 struct id_map **maps, int *num_map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_close(struct idmap_domain *dom)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static struct idmap_methods lwi_compat_methods = {
	.init = lwi_idmap_init,
	.unixids_to_sids = lwi_get_sid_from_id,
	.sids_to_unixids = lwi_get_id_from_sid,
	.set_mapping = lwi_set_mapping,
	.remove_mapping = lwi_remove_mapping,
	.dump_data = lwi_dump,
	.close_fn = lwi_close
};

NTSTATUS init_module(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION,
				  "lwicompat_v4", &lwi_compat_methods);
}
