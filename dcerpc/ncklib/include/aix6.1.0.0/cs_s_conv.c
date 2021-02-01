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
**  NAME
**
**     cs_s_conv.c
**
**  FACILITY:
**
**     Remote Procedure Call (RPC)
**     I18N Character Set Conversion Call   (RPC)
**
**  ABSTRACT:
**
**
*/
#include <commonp.h>		/* include nbase.h lbase.h internally	*/
#include <com.h>		/* definition of rpc_binding_rep_p_t	*/
#include <dce/rpcsts.h>
#include <codesets.h>		/* Data definitions for I18N NSI 
							sub-component   */
#include <stdio.h>		/* definition of NULL			*/
#include <stdlib.h>		/* definition of MB_CUR_MAX		*/
#include <iconv.h>		/* definition of iconv routines		*/
#include <langinfo.h>		/* definition of nl_langinfo routine	*/
#include <string.h>		/* definition of strncpy routine	*/
#include <errno.h>		/* definition of error numbers 		*/

#include <codesets.h>
#include <cs_s.h>		/* Private defs for code set interoperability */


void stub_conversion
(
	rpc_binding_handle_t	h,
	boolean32		server_side,
	unsigned32		from_tag,
	unsigned32		to_tag,
	byte_t			*conv_ldata,
	unsigned32		conv_l_data_len,
	byte_t			*conv_wdata,
	unsigned32		*conv_p_w_data_len,
	error_status_t		*status
)
{
	iconv_t			cd;
	byte_t			*ldata = conv_ldata;
	byte_t			*wdata = conv_wdata;
	int			size;
	int			inbytesleft;
	int			outbytesleft;
	char			*iconv_from_cd;
	char			*iconv_to_cd;
	int			i_ret;
	int			init_len;


	dce_cs_rgy_to_loc (
		from_tag,
		(idl_char **)&iconv_from_cd,
		NULL,
		NULL,
		status );

	if (*status != dce_cs_c_ok)
		return;

	dce_cs_rgy_to_loc (
		to_tag,
		(idl_char **)&iconv_to_cd,
		NULL,
		NULL,
		status );

	if (*status != dce_cs_c_ok)
		return;

	if ((cd = iconv_open(iconv_to_cd, iconv_from_cd)) == (iconv_t)-1)
	{
		*status = rpc_s_ss_incompatible_codesets;
		return;
	}

	/* Set the number of bytes left in input buffer */
	init_len = strlen((char *)ldata);
	inbytesleft = init_len;
	outbytesleft = (int)conv_l_data_len * sizeof(unsigned_char_t);

	i_ret = iconv(cd, (char **)&ldata, &inbytesleft, (char **)&wdata, &outbytesleft);

	if (i_ret)	/* Iconv returns zero when it succeed */
	{
		if (errno == EILSEQ)
			*status = rpc_s_ss_invalid_char_input;
		else if (errno = E2BIG)
			*status = rpc_s_ss_short_conv_buffer;
		else if (errno = EINVAL)
			*status = rpc_s_ss_invalid_char_input;
		i_ret = iconv_close(cd);
		return;
	}
	*wdata = '\0';	/* Guard against a stale data */

	if ((i_ret = iconv_close(cd)) == -1)
	{
		*status = rpc_s_ss_iconv_error;
		return;
	}

	if (conv_p_w_data_len != NULL)
	{
		*conv_p_w_data_len = strlen((char *)conv_wdata);
	}

	*status = rpc_s_ok;
	return;
}
