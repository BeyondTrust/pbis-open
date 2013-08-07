/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <errno.h>
#include <string.h>

#include <krb5.h>
#include <krb5/preauth_plugin.h>

static int
preauth_flags(krb5_context context, krb5_preauthtype pa_type)
{
    return PA_INFO;
}

static krb5_error_code
process_preauth(krb5_context context, void *plugin_context,
                void *request_context, krb5_get_init_creds_opt *opt,
                preauth_get_client_data_proc get_data_proc,
                struct _krb5_preauth_client_rock *rock, krb5_kdc_req *request,
                krb5_data *encoded_request_body,
                krb5_data *encoded_previous_request, krb5_pa_data *padata,
                krb5_prompter_fct prompter, void *prompter_data,
                preauth_get_as_key_proc gak_fct, void *gak_data,
                krb5_data *salt, krb5_data *s2kparams, krb5_keyblock *as_key,
                krb5_pa_data ***out_padata)
{
    krb5_error_code retval = 0;
    krb5_pa_data *pa = NULL;
    krb5_pa_data **pa_array = NULL;
    // This is the ASN.1 sequence for PAC-REQUEST: true
    krb5_octet pac_req[] = {0x30, 0x05, 0xa0, 0x03, 0x01, 0x01, 0xff};

    // The krb5_pa_data *out_padata value we set will have all elements
    // free'd by the krb5 library so ensure we allocate everything
    if (retval == 0) {
        pa_array = calloc(2, sizeof(krb5_pa_data *));

        pa = calloc(1, sizeof(krb5_pa_data));
        if (pa) 
        {
            pa->contents = calloc(7, sizeof(krb5_octet));
        }
        
        if (pa_array == NULL || pa == NULL || pa->contents == NULL) 
        {
            retval = ENOMEM;
        }
    }

    if (retval == 0) {
        pa->length = 7;
        memcpy(pa->contents, pac_req, 7 * sizeof(krb5_octet));
        pa->pa_type = KRB5_PADATA_PAC_REQUEST;

        pa_array[0] = pa;

        *out_padata = pa_array;

        pa = NULL;
        pa_array = NULL;
    }

    if (pa)
    {
        if (pa->contents) 
            free(pa->contents);

        free(pa);
    }

    if (pa_array)
        free(pa_array);
    
    return retval;
}

krb5_preauthtype supported_pa_types[] = {
    KRB5_PADATA_PAC_REQUEST, 0};

struct krb5plugin_preauth_client_ftable_v1 preauthentication_client_1 = {
    "PAC Request",           /* name */
    &supported_pa_types[0],  /* pa_type_list */
    NULL,                    /* enctype_list */
    NULL,                    /* plugin init function */
    NULL,                    /* plugin fini function */
    preauth_flags,           /* get flags function */
    NULL,                    /* request init function */
    NULL,                    /* request fini function */
    process_preauth,         /* process function */
    NULL,                    /* try_again function */
    NULL                     /* get init creds opt function */
};
