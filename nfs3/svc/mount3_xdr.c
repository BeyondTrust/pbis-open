/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "mount3.h"

bool_t
xdr_fhandle3 (XDR *xdrs, fhandle3 *objp)
{
	 if (!xdr_bytes (xdrs, (char **)&objp->fhandle3_val, (u_int *) &objp->fhandle3_len, FHSIZE3))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_dirpath (XDR *xdrs, dirpath *objp)
{
	 if (!xdr_string (xdrs, objp, MNTPATHLEN))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_name (XDR *xdrs, name *objp)
{
	 if (!xdr_string (xdrs, objp, MNTNAMLEN))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_mountstat3 (XDR *xdrs, mountstat3 *objp)
{
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_mountres3_ok (XDR *xdrs, mountres3_ok *objp)
{
	 if (!xdr_fhandle3 (xdrs, &objp->fhandle))
		 return FALSE;
	 if (!xdr_array (xdrs, (char **)&objp->auth_flavors.auth_flavors_val, (u_int *) &objp->auth_flavors.auth_flavors_len, ~0,
		sizeof (int), (xdrproc_t) xdr_int))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_mountres3 (XDR *xdrs, mountres3 *objp)
{
	 if (!xdr_mountstat3 (xdrs, &objp->fhs_status))
		 return FALSE;
	switch (objp->fhs_status) {
	case MNT3_OK:
		 if (!xdr_mountres3_ok (xdrs, &objp->mountres3_u.mountinfo))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}

bool_t
xdr_mountlist (XDR *xdrs, mountlist *objp)
{
	 if (!xdr_pointer (xdrs, (char **)objp, sizeof (struct mountbody), (xdrproc_t) xdr_mountbody))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_mountbody (XDR *xdrs, mountbody *objp)
{
	 if (!xdr_name (xdrs, &objp->ml_hostname))
		 return FALSE;
	 if (!xdr_dirpath (xdrs, &objp->ml_directory))
		 return FALSE;
	 if (!xdr_mountlist (xdrs, &objp->ml_next))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_groups (XDR *xdrs, groups *objp)
{
	 if (!xdr_pointer (xdrs, (char **)objp, sizeof (struct groupnode), (xdrproc_t) xdr_groupnode))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_groupnode (XDR *xdrs, groupnode *objp)
{
	 if (!xdr_name (xdrs, &objp->gr_name))
		 return FALSE;
	 if (!xdr_groups (xdrs, &objp->gr_next))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_exports (XDR *xdrs, exports *objp)
{
	 if (!xdr_pointer (xdrs, (char **)objp, sizeof (struct exportnode), (xdrproc_t) xdr_exportnode))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_exportnode (XDR *xdrs, exportnode *objp)
{
	 if (!xdr_dirpath (xdrs, &objp->ex_dir))
		 return FALSE;
	 if (!xdr_groups (xdrs, &objp->ex_groups))
		 return FALSE;
	 if (!xdr_exports (xdrs, &objp->ex_next))
		 return FALSE;
	return TRUE;
}
