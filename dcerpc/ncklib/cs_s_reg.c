/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
/*
 */
/*
**
**  NAME
**
**      cs_s_reg.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) OSF Code Set Registry Access routines
**
**  ABSTRACT:
**
**      Code set interoperability requires a common way to recognize the
**      supported code sets for client and server.  OSF code set registry
**	provides that functionality.  OSF code set registry is a binary file
**	which is produced by 'csrc' (Code set registry compiler).
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <dce/rpc.h>
#include <dce/csrc.h>
#include <unistd.h>
#include <fcntl.h>
#include <commonp.h>
#include <dce/dce_cf.h>		/* Access to the backing store library */
#include <langinfo.h>



/* 
** Local globals for code set registry access.  
** Code set registry is pointed by rpc_g_codesets_list.
*/
static boolean		rpc_g_codesets_did_read;
static dcethread_mutex	rpc_g_codesets_mutex;
static dcethread_oncectl rpc_g_codesets_once_block = DCETHREAD_ONCE_INIT;
static entry_t		*rpc_g_codesets_list;
static entry_t		*rpc_g_codesets_effective_ids;
static entry_t		**rpc_g_codesets_sort_by_priority;
static entry_t		**rpc_g_codesets_sort_by_name;
static entry_t		**rpc_g_codesets_sort_by_id;
static int		rpc_g_codesets_entry_count;
static int		rpc_g_codesets_effective_count;
static error_status_t	rpc_g_codesets_status;

/*
**
** Iterative Insertion Sort for code set name.
**
*/
PRIVATE
void name_sort
(
	entry_t		**codesets, 
	int		entry_count
)
{
	int	i, j, k;
	entry_t	*temp;

	if (entry_count <= 1)
		return;

	for (i = 1; i < entry_count; i++)
	{
		temp = codesets[i];
		j = i - 1;
		while (j >= 0)
		{
			if ((k = strcoll(codesets[j]->code_set_name, (const char *)temp->code_set_name)) > 0)
			{
				codesets[j + 1] = codesets[j];
				codesets[j] = temp;
			}
			j--;
		}
	}
}

/*
**
** Iterative Insertion Sort for priority
**
*/
PRIVATE
void priority_sort
(
	entry_t		**codesets, 
	int		entry_count
)
{
	int	i, j, k;
	entry_t	*temp;

	if (entry_count <= 1)
		return;

	for (i = 1; i < entry_count; i++)
	{
		temp = codesets[i];
		j = i - 1;
		while (j >= 0)
		{
			if (codesets[j]->priority > temp->priority)
			{
				codesets[j + 1] = codesets[j];
				codesets[j] = temp;
			}
			j--;
		}
	}
}


/*
**
** Binary Search for code set value
**
*/
PRIVATE
void c_binary_search
(
	entry_t		**codesets, 
	int		low, 
	int		high, 
	long		key_value,
	entry_t		**code_entry
)
{
	int	middle;
	long	code1, code2;

	if (key_value < 0)		/* little trick for large number */
		code1 = key_value * -1;
	else
		code1 = key_value;

	if (low <= high)
	{
		middle = (low + high) / 2;

		if (codesets[middle]->code_set < 0)
			code2 = codesets[middle]->code_set * -1;
		else
			code2 = codesets[middle]->code_set;

		if (code1 == code2)
		{
			*code_entry = codesets[middle];
			return;
		}
		else
		{
			if (code1 < code2)
				c_binary_search(codesets, low, middle-1, key_value, code_entry);
			else
				c_binary_search(codesets, middle+1, high, key_value, code_entry);
		}
	}
}


/*
**
** Binary Search for code set name
**
*/
PRIVATE
void n_binary_search
(
	entry_t		**codesets, 
	int		low, 
	int		high, 
	char		*key_name,
	entry_t		**code_entry
)
{
	int	middle, k;

	if (low <= high)
	{
		middle = (low + high) / 2;

		if ((k = strcoll(key_name, codesets[middle]->code_set_name)) == 0)
		{
			*code_entry = codesets[middle];
			return;
		}
		else
		{
			if (k < 0)
				n_binary_search(codesets, low, middle-1, key_name, code_entry);
			else
				n_binary_search(codesets, middle+1, high, key_name, code_entry);
		}
	}
}


/*
**++
**  ROUTINE NAME:           rpc__codesets_really_read_file
**
**  SCOPE:                  PRIVATE
**
**  DESCRIPTION:
**
**  Internal routine to read OSF code set registry data into a memory.
**  To avoid the unnecessary overhead, this routine only read the code set
**  registry data once, when code set registry is first accessed.
**  This function takes no arguments and returns no data.
**
**--
*/
PRIVATE
rpc__codesets_really_read_file
(
	void
)
{
	int	i, j, k;
	entry_t	*ep;
	entry_t	*effective_ep;
	int	CsrFile;
	char	*code_set_registry_file;

	/* 
	** Open the code set registry file.  The default path is
	** "/usr/lib/nls/csr/code_set_registry.db"
	*/ 
	dce_cf_get_csrgy_filename(&code_set_registry_file, &rpc_g_codesets_status);
	if (rpc_g_codesets_status != dce_cf_st_ok)
	{
		return;
	}

	CsrFile = open(code_set_registry_file, O_RDONLY, 0);
	free (code_set_registry_file);
	if (CsrFile == -1)
	{
		rpc_g_codesets_status = dce_cs_c_cannot_open_file;
		return;
	}

	if ((read (CsrFile, (char *)(&rpc_g_codesets_entry_count), sizeof(rpc_g_codesets_entry_count))) == -1)
	{
		rpc_g_codesets_status = dce_cs_c_cannot_read_file;
		return;
	}

	if ((read (CsrFile, (char *)(&rpc_g_codesets_effective_count), sizeof(rpc_g_codesets_entry_count))) == -1)
	{
		rpc_g_codesets_status = dce_cs_c_cannot_read_file;
		return;
	}

	if ((rpc_g_codesets_list = (entry_t *)malloc(sizeof(entry_t) * rpc_g_codesets_entry_count)) == NULL)
	{
		rpc_g_codesets_status = dce_cs_c_cannot_allocate_memory;
		return;
	}

	if ((rpc_g_codesets_effective_ids = (entry_t *)malloc(sizeof(entry_t) * rpc_g_codesets_effective_count)) == NULL)
	{
		rpc_g_codesets_status = dce_cs_c_cannot_allocate_memory;
		return;
	}

	ep = rpc_g_codesets_list;
	i = rpc_g_codesets_entry_count;
	effective_ep = rpc_g_codesets_effective_ids;

	while (i--)
	{
		if ((read (CsrFile, (char *)(&ep->code_set), sizeof(ep->code_set))) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		if ((read (CsrFile, (char *)(&ep->code_name_len), sizeof(ep->code_name_len))) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		if ((ep->code_set_name = (char *)malloc(ep->code_name_len + 1)) == NULL)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_allocate_memory;
			return;
		}

		if ((read (CsrFile, (char *)ep->code_set_name, ep->code_name_len + 1)) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		if ((read (CsrFile, (char *)(&ep->char_sets_num), sizeof(ep->char_sets_num))) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		for (j = 0; j < ep->char_sets_num; j++)
		{
			if ((read (CsrFile, (char *)(&ep->char_sets[j]), sizeof(ep->char_sets[j]))) == -1)
			{
				rpc_g_codesets_status = dce_cs_c_cannot_read_file;
				return;
			}
		}

		if ((read (CsrFile, (char *)(&ep->max_bytes), sizeof(ep->max_bytes))) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		if ((read (CsrFile, (char *)(&ep->priority), sizeof(ep->priority))) == -1)
		{
			rpc_g_codesets_status = dce_cs_c_cannot_read_file;
			return;
		}

		if ((k = strcoll(ep->code_set_name, "NONE")) != 0)
		{
			effective_ep->code_set      =  ep->code_set;
			effective_ep->code_name_len =  ep->code_name_len;
			effective_ep->code_set_name =  ep->code_set_name;
			effective_ep->char_sets_num =  ep->char_sets_num;
			for (j = 0; j < ep->char_sets_num; j++)
				effective_ep->char_sets[j] = ep->char_sets[j];
			effective_ep->max_bytes     =  ep->max_bytes;
			effective_ep->priority      =  ep->priority;

			effective_ep++;
		}

		ep++;
	}

	close (CsrFile);

	rpc_g_codesets_status = rpc_s_ok;

	rpc_g_codesets_did_read = TRUE;
}

/*
**++
**  ROUTINE NAME:           rpc__codesets_read_registry_file
**
**  SCOPE:                  PRIVATE
**
**  DESCRIPTION:
**
**  Internal routine to allocate OSF code set registry data into a memory.
**  rpc__codesets_really_read_file() is called to actually perform
**  reading data from a registry file.  Code set registry is sorted by
**  code set id and code set name.  Local global pointers point to each
**  sorted memory.
**
**--
*/
PRIVATE
rpc__codesets_read_registry_file
(
	error_status_t	*status
)
{
	entry_t		**sort_name_codesets;
	entry_t		**sort_name_save;
	entry_t		**sort_id_codesets;
	entry_t		**sort_id_save;
	entry_t		**sort_priority_codesets;
	entry_t		**sort_priority_save;
	int		i;
	entry_t		*ep;

	if (!rpc_g_codesets_did_read)
	{
		dcethread_once_throw(&rpc_g_codesets_once_block,
		(dcethread_initroutine)rpc__codesets_really_read_file);

		if (rpc_g_codesets_status != rpc_s_ok)
		{
			*status = rpc_g_codesets_status;
			return;
		}
	}

	/* 
	** Sort the code set registry file by code set name
	**/
	if ((sort_name_save = (entry_t **)malloc(sizeof(entry_t *) * rpc_g_codesets_entry_count)) == NULL)
	{
		*status = dce_cs_c_cannot_allocate_memory;
		return;
	}

	ep = rpc_g_codesets_list;
	i = rpc_g_codesets_entry_count;
	sort_name_codesets = sort_name_save;
	while (i--)
	{
		*sort_name_codesets++ = ep++;
	}
	rpc_g_codesets_sort_by_name = sort_name_save;
	sort_name_codesets = sort_name_save;
	name_sort(sort_name_codesets, rpc_g_codesets_entry_count);

	/* 
	** Sort the effective code set by priority
	**/
	if ((sort_priority_save = (entry_t **)malloc(sizeof(entry_t *) * rpc_g_codesets_effective_count)) == NULL)
	{
		*status = dce_cs_c_cannot_allocate_memory;
		return;
	}
	ep = rpc_g_codesets_effective_ids;
	i = rpc_g_codesets_effective_count;
	sort_priority_codesets = sort_priority_save;
	while (i--)
	{
		*sort_priority_codesets++ = ep++;
	}
	rpc_g_codesets_sort_by_priority = sort_priority_save;
	sort_priority_codesets = sort_priority_save;
	priority_sort(sort_priority_codesets, rpc_g_codesets_effective_count); 

	/* 
	** Allocate an array for pointers to entry_t.  Code set registry
	** is already sorted by code set id value.
	*/
	if ((sort_id_save = (entry_t **)malloc(sizeof(entry_t *) * rpc_g_codesets_entry_count)) == NULL)
	{
		*status = dce_cs_c_cannot_allocate_memory;
		return;
	}
	ep = rpc_g_codesets_list;
	i = rpc_g_codesets_entry_count;
	sort_id_codesets = sort_id_save;
	while (i--)
	{
		*sort_id_codesets++ = ep++;
	}
	rpc_g_codesets_sort_by_id = sort_id_save;

	*status = rpc_s_ok;

}


/*
**++
**  ROUTINE NAME:           dce_cs_loc_to_rgy
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Convert code set name (in string) to code set id (integer).
**  When rgy_char_sets_number is NULL, no character sets values
**  will be returned.  Character sets are mainly used to evaluate
**  code set compatibility.
**
**  INPUTS:
**
**      local_code_set_name	OS specific name for the code set
**
**
**  INPUT/OUPUTS:	    NONE
**
**
**  OUTPUTS:
**
**	rgy_code_set_value	Registerd code set id
**
**	rgy_char_sets_number	Number of character sets supported by
**				the code set
**
**	rgy_char_sets_value	Array of character set IDs supported by
**				the code set
**
**      status              The result of the operation. One of:
**                              dce_cs_c_ok
**                              dce_cs_c_notfound
**                              dce_cs_c_cannot_allocate_memory
**				status from rpc__codesets_read_registry_file
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void dce_cs_loc_to_rgy
(
	idl_char		*local_code_set_name,
	unsigned32		*rgy_code_set_value,
	unsigned16		*rgy_char_sets_number,
	unsigned16		**rgy_char_sets_value,
	error_status_t		*status
)
{
	entry_t		**epp;
	entry_t		*found = NULL;
	int		i;
	unsigned16	*char_array;


	CODING_ERROR (status);

	rpc__codesets_read_registry_file(status);
	if (*status != rpc_s_ok)
	{
		return;
	}

	epp = rpc_g_codesets_sort_by_name;

	/* binary search */
	n_binary_search(epp, 0, rpc_g_codesets_entry_count-1, (char *)local_code_set_name, &found); 

	if (found == NULL)
	{
		*status = dce_cs_c_unknown;
		return;
	}

	if (rgy_char_sets_number != NULL)
	{
		*rgy_char_sets_number = found->char_sets_num;
	}

	if (rgy_char_sets_value != NULL)
	{
		if ((char_array = (unsigned16 *)malloc(sizeof(unsigned16) * found->char_sets_num)) == NULL)
		{
			*status = dce_cs_c_cannot_allocate_memory;
			return;
		}

		*rgy_char_sets_value = char_array;

		for (i = 0; i < found->char_sets_num; i++)
		{
			*char_array++ = found->char_sets[i];
		}
	}
	*rgy_code_set_value = (unsigned32)found->code_set;

	*status = dce_cs_c_ok;
	return;
}


/*
**++
**  ROUTINE NAME:           dce_cs_rgy_to_loc
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Convert code set id (integer) to code set name (in string).
**  When rgy_char_sets_number is NULL, no character sets values
**  will be returned.  Character sets are mainly used to evaluate
**  code set compatibility.
**
**  INPUTS:
**
**	rgy_code_set_value	Registerd code set id
**
**
**  INPUT/OUPUTS:	    NONE
**
**
**  OUTPUTS:
**
**      local_code_set_name	OS specific name for the code set
**
**	rgy_char_sets_number	Number of character sets supported by
**				the code set
**
**	rgy_char_sets_value	Array of character set IDs supported by
**				the code set
**
**      status              The result of the operation. One of:
**                              dce_cs_c_ok
**                              dce_cs_c_notfound
**                              dce_cs_c_cannot_allocate_memory
**				status from rpc__codesets_read_registry_file
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void dce_cs_rgy_to_loc
(
	unsigned32		rgy_code_set_value,
	idl_char 		**local_code_set_name,
	unsigned16		*rgy_char_sets_number,
	unsigned16		**rgy_char_sets_value,
	error_status_t		*status
)
{
	entry_t		**epp;
	entry_t		*found = NULL;
	int		i;
	unsigned16	*char_array;


	CODING_ERROR (status);

	rpc__codesets_read_registry_file(status);
	if (*status != rpc_s_ok)
	{
		return;
	}

	epp = rpc_g_codesets_sort_by_id;
	i = rpc_g_codesets_entry_count;

	/* binary search */
	c_binary_search(epp, 0, rpc_g_codesets_entry_count-1, rgy_code_set_value, &found); 

	if (found == NULL)
	{
		*status = dce_cs_c_unknown;
		return;
	}

	if ((i = strcoll(found->code_set_name, "NONE")) == 0)
	{
		*status = dce_cs_c_notfound;
		return;
	}

	if (rgy_char_sets_number != NULL)
	{
		*rgy_char_sets_number = found->char_sets_num;
	}

	if (rgy_char_sets_value != NULL)
	{
		if ((char_array = (unsigned16 *)malloc(sizeof(unsigned16) * found->char_sets_num)) == NULL)
		{
			*status = dce_cs_c_cannot_allocate_memory;
			return;
		}

		*rgy_char_sets_value = char_array;

		for (i = 0; i < found->char_sets_num; i++)
		{
			*char_array++ = found->char_sets[i];
		}
	}
	*local_code_set_name = (idl_char *)found->code_set_name;

	*status = dce_cs_c_ok;
	return;
}


/*
**++
**  ROUTINE NAME:           rpc_rgy_get_max_bytes
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Search maximum number of bytes for a tag specified.  The tag
**  is the code set id number from OSF code set registry.
**  This routine is mainly used by a stub to calculate a size of
**  necessary conversion buffer.
**
**  INPUTS:
**
**	tag			Registerd code set id
**
**
**  INPUT/OUPUTS:	    NONE
**
**
**  OUTPUTS:
**
**      max_bytes		Maximum number of bytes needed to encode
**				a character in the code set
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**                              dce_cs_c_notfound
**				status from rpc__codesets_read_registry_file
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_rgy_get_max_bytes
(
	unsigned32		tag,
	unsigned16		*max_bytes,
	error_status_t		*status
)
{
	entry_t		**epp;
	entry_t		*found = NULL;
	int		i;
	unsigned16	*char_array;


	CODING_ERROR (status);

	rpc__codesets_read_registry_file(status);
	if (*status != rpc_s_ok)
	{
		return;
	}

	epp = rpc_g_codesets_sort_by_id;
	i = rpc_g_codesets_entry_count;

	/* binary search */
	c_binary_search(epp, 0, rpc_g_codesets_entry_count-1, tag, &found); 

	if (found == NULL)
	{
		*status = dce_cs_c_unknown;
		return;
	}

	if ((i = strcoll(found->code_set_name, "NONE")) == 0)
	{
		*status = dce_cs_c_notfound;
		return;
	}

	*max_bytes = found->max_bytes;

	*status = rpc_s_ok;
	return;
}

/*
**++
**  ROUTINE NAME:           rpc_rgy_get_codesets
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  This an OS dependent routine.  It will determine which code sets
**  are supported by a current machine, and returns a list of code sets.
**  The first code set in a list should be a current locale's code set.
**
**
**  INPUTS: none
**      
**  INPUT/OUPUTS:
**
**	args 		    Actually points to 'rpc_cs_codeset_i14y_data_p'
**			    data type.
**
**  OUTPUTS: none
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_rgy_get_codesets
(
	rpc_codeset_mgmt_p_t	*codesets_p,
	error_status_t		*status
)
{
	int             i;
	entry_t		**epp;
	char		*current_codeset;
	unsigned32	current_rgy_codeset;

	CODING_ERROR (status);

	rpc__codesets_read_registry_file(status);
	if (*status != rpc_s_ok)
	{
		return;
	}

	epp = rpc_g_codesets_sort_by_priority;

	RPC_MEM_ALLOC (
		*codesets_p,
		rpc_codeset_mgmt_p_t,
		sizeof(rpc_codeset_mgmt_t) + 
		  (sizeof(rpc_cs_c_set_t) * (rpc_g_codesets_effective_count - 1)),
		RPC_C_MEM_CDS_ATTR,
		RPC_C_MEM_WAITOK);

	(*codesets_p)->count = rpc_g_codesets_effective_count;

	current_codeset = nl_langinfo(CODESET);
	dce_cs_loc_to_rgy(
		(unsigned_char_p_t)current_codeset,
		&current_rgy_codeset,
		NULL, NULL,
		status);

	if (*status != dce_cs_c_ok)
	{
		/* codeset registry error */
		*status = rpc_s_ok;
		return;
	}

	/* 
	 * The top of the list is current locale's code set
	 */
	(*codesets_p)->codesets[0].c_set = current_rgy_codeset;

	for (i = 1; i < rpc_g_codesets_effective_count; i++)
	{
		/*
		**  This logic assumes current locale's code set is one of
		**  the supported code set.  It should be.
		*/
		if ((*epp)->code_set != (*codesets_p)->codesets[0].c_set)
		{
			(*codesets_p)->codesets[i].c_set = (*epp)->code_set;

			(*codesets_p)->codesets[i].c_max_bytes = (*epp)->max_bytes;
		}
		else
		{
			(*codesets_p)->codesets[0].c_max_bytes = (*epp)->max_bytes;
			i--;
		}

		epp++;
	}

	*status = rpc_s_ok;
}
