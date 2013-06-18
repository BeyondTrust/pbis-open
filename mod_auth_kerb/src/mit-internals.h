/*
 * Copyright (C) 1989,1990,1991,1992,1993,1994,1995,2000,2001, 2003 by the Massachusetts Institute of Technology,
 * Cambridge, MA, USA.  All Rights Reserved.
 *
 * This software is being provided to you, the LICENSEE, by the
 * Massachusetts Institute of Technology (M.I.T.) under the following
 * license.  By obtaining, using and/or copying this software, you agree
 * that you have read, understood, and will comply with these terms and
 * conditions:
 *
 * Export of this software from the United States of America may
 * require a specific license from the United States Government.
 * It is the responsibility of any person or organization contemplating
 * export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify and distribute
 * this software and its documentation for any purpose and without fee or
 * royalty is hereby granted, provided that you agree to comply with the
 * following copyright notice and statements, including the disclaimer, and
 * that the same appear on ALL copies of the software and documentation,
 * including modifications that you make for internal use or for
 * distribution:
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", AND M.I.T. MAKES NO REPRESENTATIONS
 * OR WARRANTIES, EXPRESS OR IMPLIED.  By way of example, but not
 * limitation, M.I.T. MAKES NO REPRESENTATIONS OR WARRANTIES OF
 * MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF
 * THE LICENSED SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY THIRD PARTY
 * PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
 *
 * The name of the Massachusetts Institute of Technology or M.I.T. may NOT
 * be used in advertising or publicity pertaining to distribution of the
 * software.  Title to copyright in this software and any associated
 * documentation shall at all times remain with M.I.T., and USER agrees to
 * preserve same.
 *
 * Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 */

/*
 * Copyright (C) 1998 by the FundsXpress, INC.
 *
 * All rights reserved.
 *
 * Export of this software from the United States of America may require
 * a specific license from the United States Government.  It is the
 * responsibility of any person or organization contemplating export to
 * obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of FundsXpress. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  FundsXpress makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * Copyright 1993 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _MIT_INTERNALS_H_
#define _MIT_INTERNALS_H_

/* must be included after krb5.h to override definitons from there */

/*
 * MIT Kerberos 1.3.x replay cache implementation causes major problems
 * with Microsoft Kerberos5 implementation by incorrectly detecting
 * Microsoft authenticators as replays. The problem is being worked on
 * by both MIT and Microsoft but until a definite fix is available, we
 * must disable the replay cache in order to work with Microsoft clients.
 * The only working way to do this seems to be overriding the function
 * that stores authenticators in replay cache with one that does nothing.
 * Note that disabling replay cache is potentially unsecure.
 */

/* Definition from MIT krb5-1.3.3 krb5.h */
typedef struct _krb5_donot_replay_internal {
    krb5_magic magic;
    char *server;                       /* null-terminated */
    char *client;                       /* null-terminated */
    krb5_int32 cusec;
    krb5_timestamp ctime;
} krb5_donot_replay_internal;

/* Definitions from MIT krb5-1.3.3 k5-int.h */
struct _krb5_rc_ops_internal {
    krb5_magic magic;
    char *type;
    krb5_error_code (KRB5_CALLCONV *init)
        (krb5_context, krb5_rcache,krb5_deltat); /* create */
    krb5_error_code (KRB5_CALLCONV *recover)
        (krb5_context, krb5_rcache); /* open */
    krb5_error_code (KRB5_CALLCONV *destroy)
        (krb5_context, krb5_rcache);
    krb5_error_code (KRB5_CALLCONV *close)
        (krb5_context, krb5_rcache);
    krb5_error_code (KRB5_CALLCONV *store)
        (krb5_context, krb5_rcache,krb5_donot_replay_internal *);
    krb5_error_code (KRB5_CALLCONV *expunge)
        (krb5_context, krb5_rcache);
    krb5_error_code (KRB5_CALLCONV *get_span)
        (krb5_context, krb5_rcache,krb5_deltat *);
    char *(KRB5_CALLCONV *get_name)
        (krb5_context, krb5_rcache);
    krb5_error_code (KRB5_CALLCONV *resolve)
        (krb5_context, krb5_rcache, char *);
};

typedef struct _krb5_rc_ops_internal krb5_rc_ops_internal;

/* Definitions from MIT krb5-1.3.3 rc_dfl.h */
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_init
        (krb5_context,
                   krb5_rcache,
                   krb5_deltat);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_recover
        (krb5_context,
                   krb5_rcache);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_destroy
        (krb5_context,
                   krb5_rcache);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_close
        (krb5_context,
                   krb5_rcache);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_expunge
        (krb5_context,
                   krb5_rcache);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_get_span
        (krb5_context,
                   krb5_rcache,
                   krb5_deltat *);
extern char * KRB5_CALLCONV krb5_rc_dfl_get_name
        (krb5_context,
                   krb5_rcache);
extern krb5_error_code KRB5_CALLCONV krb5_rc_dfl_resolve
        (krb5_context,
                   krb5_rcache,
                   char *);

/* Definition from MIT krb5-1.3.3 k5-int.h */
/* kouril: use the _internal suffix in order to avoid conflicts with 
 * the definition in krb5.h */
struct krb5_rc_st_internal {
	krb5_magic magic;
	const struct _krb5_rc_ops_internal *ops;
	krb5_pointer data;
};

typedef struct krb5_rc_st_internal *krb5_rcache_internal;

/* Definitions from MIT krb5-1.3.3 gssapiP_krb5.h */
typedef struct _krb5_gss_cred_id_rec {
	/* name/type of credential */
	gss_cred_usage_t usage;
        krb5_principal princ;        /* this is not interned as a gss_name_t */
	int prerfc_mech;
	int rfc_mech;

        /* keytab (accept) data */
        krb5_keytab keytab;
	krb5_rcache_internal rcache;
	
        /* ccache (init) data */
	krb5_ccache ccache;
	krb5_timestamp tgt_expire;
} krb5_gss_cred_id_rec, *krb5_gss_cred_id_t;

#endif /* _MIT_INTERNALS_H_ */
