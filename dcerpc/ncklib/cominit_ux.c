/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
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
 * Copyright (c) 1983, 1993
 *        The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the University of
 *        California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 */
/*
**
**  NAME
**
**      cominit_ux.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Initialization Service support routines for Unix platforms.
**
*/

#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <cominitp.h>

/* Determine shared library extension
 * FIXME: move this into a centralized location
 */

#if (defined(_HPUX) || defined (__hpux)) && defined (__hppa)
#define DSO_EXT ".sl"
#else
#define DSO_EXT ".so"
#endif

#if HAVE_DLFCN_H
#include <dirent.h>
#include <dlfcn.h>

/* load protocols before naf, then auth */
#ifndef HAVE_SCANDIR
static int scandir(const char *, struct dirent ***, int (*)(const struct dirent *), int (*)(const void *, const void *));
#endif
static int sort_modules(const void* a, const void *b);
static int select_module(const struct dirent * dirent);

static int sort_modules(const void* a, const void* b)
{
        int pri_a, pri_b;

        switch((*(const struct dirent**)a)->d_name[3])        {
                case 'p': pri_a = 1; break;
                case 'n': pri_a = 2; break;
                case 'a': pri_a = 3; break;
                default: pri_a = 4;
        }
        switch((*(const struct dirent**)b)->d_name[3])        {
                case 'p': pri_b = 1; break;
                case 'n': pri_b = 2; break;
                case 'a': pri_b = 3; break;
                default: pri_b = 4;
        }
        if (pri_a == pri_b)
                return 0;
        if (pri_a < pri_b)
                return -1;
        return 1;
}

static int select_module(const struct dirent * dirent)
{
        int len = strlen(dirent->d_name);
        const char * module_types[] = {"libnaf_", "libauth_", "libprot_", NULL};
        int i;

        for (i=0; module_types[i] != NULL; i++)        {
                if (strncmp(dirent->d_name, module_types[i], strlen(module_types[i])) == 0)        {
                        /* prefix matches; now check for a matching suffix */
                        if (strcmp(&dirent->d_name[len - 3], DSO_EXT) == 0)
                                return 1;
                }
        }
        /* reject */
        return 0;
}
#endif

/* register an auth protocol */
PRIVATE void rpc__register_authn_protocol(rpc_authn_protocol_id_elt_p_t auth, int number)
{
        int i;
        for (i=0; i<number; i++)        {
                RPC_DBG_PRINTF(rpc_es_dbg_general, 1, ("Register authn protocol 0x%0x\n", auth[i].authn_protocol_id));        

                memcpy(&rpc_g_authn_protocol_id[auth[i].authn_protocol_id],
                                &auth[i],
                                sizeof(rpc_authn_protocol_id_elt_t)
                                );
        }
}

/* register a protocol sequence with the runtime */
PRIVATE void rpc__register_protseq(rpc_protseq_id_elt_p_t elt, int number)
{
        int i;
        for (i=0; i<number; i++)        {
                RPC_DBG_PRINTF(rpc_es_dbg_general, 1, ("Register protseq 0x%0x %s\n", elt[i].rpc_protseq_id, elt[i].rpc_protseq));        
                memcpy(&rpc_g_protseq_id[elt[i].rpc_protseq_id],
                                &elt[i],
                                sizeof(rpc_protseq_id_elt_t));
        }
}

/* register a tower protocol id */
PRIVATE void rpc__register_tower_prot_id(rpc_tower_prot_ids_p_t tower_prot, int number)
{
        int i;
        for (i=0; i<number; i++) {
                rpc_tower_prot_ids_p_t tower = &tower_prot[i];
                
                RPC_DBG_PRINTF(rpc_es_dbg_general, 1,
                                ("Register tower protocol for %s\n",
                                         rpc_g_protseq_id[tower->rpc_protseq_id].rpc_protseq
                                )
                );        

                memcpy(&rpc_g_tower_prot_ids[rpc_g_tower_prot_id_number],
                                tower, sizeof(rpc_tower_prot_ids_t));

                rpc_g_tower_prot_id_number++;
        }
}

PRIVATE void rpc__register_protocol_id(rpc_protocol_id_elt_p_t prot, int number)
{
        int i;
        for (i=0; i<number; i++)        {
                RPC_DBG_PRINTF(rpc_es_dbg_general, 1,
                                ("Register protocol id 0x%x\n", prot[i].rpc_protocol_id));        


                memcpy(&rpc_g_protocol_id[prot[i].rpc_protocol_id],
                                &prot[i],
                                sizeof(rpc_protocol_id_elt_t));
        }
}

PRIVATE void rpc__register_naf_id(rpc_naf_id_elt_p_t naf, int number)
{
        int i;
        for (i=0; i < number; i++)        {
                RPC_DBG_PRINTF(rpc_es_dbg_general, 1,
                                ("Register network address family id 0x%x\n", naf[i].naf_id));        


                memcpy(&rpc_g_naf_id[naf[i].naf_id],
                                &naf[i],
                                sizeof(rpc_naf_id_elt_t));
        }
}

void rpc__cn_init_func(void);
void rpc__dg_init_func(void);
void rpc__ip_naf_init_func(void);
void rpc__np_naf_init_func(void);
void rpc__http_naf_init_func(void);
void rpc__gssauth_init_func(void);
void rpc__ntlmauth_init_func(void);
void rpc__schnauth_init_func(void);

static void (*rpc__g_static_modules[])(void) =
{
#ifdef ENABLE_PROT_NCACN
    rpc__cn_init_func,
#endif
#ifdef ENABLE_PROT_NCADG
    rpc__dg_init_func,
#endif
#ifdef ENABLE_NAF_IP
    rpc__ip_naf_init_func,
#endif
#ifdef ENABLE_NAF_NP
    rpc__np_naf_init_func,
#endif
#ifdef ENABLE_NAF_HTTP
    rpc__http_naf_init_func,
#endif
#ifdef ENABLE_AUTH_GSS_NEGOTIATE
    rpc__gssauth_init_func,
#endif
#ifdef ENABLE_AUTH_NTLMSSP
    rpc__ntlmauth_init_func,
#endif
#ifdef ENABLE_AUTH_SCHANNEL
    rpc__schnauth_init_func
#endif
};

PRIVATE void rpc__load_modules(void)
{
#if HAVE_DLFCN_H
        struct dirent **namelist = NULL;
        int i, n;
        void * image;
        char buf[PATH_MAX];

        memset(rpc_g_protseq_id, 0, sizeof(rpc_g_protseq_id));
        memset(rpc_g_naf_id, 0, sizeof(rpc_g_naf_id));
        memset(rpc_g_authn_protocol_id, 0, sizeof(rpc_g_authn_protocol_id));

        rpc_g_authn_protocol_id[rpc_c_authn_none].authn_protocol_id = rpc_c_authn_none;
        rpc_g_authn_protocol_id[rpc_c_authn_none].dce_rpc_authn_protocol_id = dce_c_rpc_authn_protocol_none;
        
        /* Load static modules */
        for (i = 0; i < sizeof(rpc__g_static_modules) / sizeof(rpc__g_static_modules[0]); i++)
        {
            rpc__g_static_modules[i]();
        }

        n = scandir(IMAGE_DIR, &namelist, (void*) select_module, (void*) sort_modules);
        for (i = 0; i < n; i++)
        {
                int flags = 0;

                sprintf(buf, "%s/%s", IMAGE_DIR, namelist[i]->d_name);

                RPC_DBG_PRINTF(rpc_es_dbg_general, 1, ("Loading module %s\n", buf));        

#ifdef RTLD_LAZY
                /* Allow lazy symbol binding */
                flags |= RTLD_LAZY;
#endif /* RTLD_LAZY */

#ifdef RTLD_MEMBER
                /* Allow file name to specify an archive member */
                flags |= RTLD_MEMBER;
#endif /* RTLD_MEMBER */

                image = dlopen(buf, flags);
                if (image != NULL)
                {
                        void (*init_func)(void);
                        
                        init_func = dlsym(image, "rpc__module_init_func");
                        if (init_func != NULL)
                                (*init_func)();
                        else                        
                                dlclose(image);
                }
                else
                {
                        RPC_DCE_SVC_PRINTF ((
                                        DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
                                        rpc_svc_general,
                                        svc_c_sev_fatal | svc_c_action_abort,
                                        rpc_m_call_failed,
                                        "rpc__load_modules",
                                        dlerror() ));
                }
		free(namelist[i]);
        }
	if (namelist != NULL)
	{
	        free(namelist);
	}
#endif /* HAVE_DLFCN_H */
}



/* 
 * Routines for loading Network Families and Protocol Families as shared
 * images.  i.e., VMS
 */

PRIVATE rpc_naf_init_fn_t  rpc__load_naf
(
  rpc_naf_id_elt_p_t      naf ATTRIBUTE_UNUSED,
  unsigned32              *status
)
{
        *status = rpc_s_ok;
        return((rpc_naf_init_fn_t)NULL);
}


PRIVATE rpc_prot_init_fn_t  rpc__load_prot
(
    rpc_protocol_id_elt_p_t prot ATTRIBUTE_UNUSED,
    unsigned32              *status
)
{
    *status = rpc_s_ok;
    return((rpc_prot_init_fn_t)NULL);
}

PRIVATE rpc_auth_init_fn_t  rpc__load_auth
(
    rpc_authn_protocol_id_elt_p_t auth ATTRIBUTE_UNUSED,
    unsigned32              *status
)
{
    *status = rpc_s_ok;
    return((rpc_auth_init_fn_t)NULL);
}

#if HAVE_DLFCN_H
#ifndef HAVE_SCANDIR
/*
 * Scan the directory dirname calling selectfn to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

/*
 * The DIRSIZ macro is the minimum record length which will hold the directory
 * entry.  This requires the amount of space in struct dirent without the
 * d_name field, plus enough space for the name and a terminating nul byte
 * (dp->d_namlen + 1), rounded up to a 4 byte boundary.
 */

static int
scandir(dirname, namelist, selectfn, dcomp)
        const char *dirname;
        struct dirent ***namelist;
        int (*selectfn)(const struct dirent *);
        int (*dcomp)(const void *, const void *);
{
        struct dirent *d, *p, **names = NULL;
        size_t nitems = 0;
        struct stat stb;
        unsigned long arraysz;
        DIR *dirp;

        if ((dirp = opendir(dirname)) == NULL)
                return(-1);

	if (stat(dirname, &stb))
	  return(-1);

        /*
         * estimate the array size by taking the size of the directory file
         * and dividing it by a multiple of the minimum size entry.
         */
        arraysz = (stb.st_size / 24);
        names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
        if (names == NULL)
                goto fail;

        while ((d = readdir(dirp)) != NULL) {
                if (selectfn != NULL && !(*selectfn)(d))
                        continue;        /* just selected names */
                /*
                 * Make a minimum size copy of the data
                 */
                p = (struct dirent *)malloc(sizeof(*p) + strlen(d->d_name));
                if (p == NULL)
                        goto fail;
                p->d_ino = d->d_ino;
                p->d_off = d->d_off;
                p->d_reclen = d->d_reclen;
                strcpy(p->d_name, d->d_name);
                /*
                 * Check to make sure the array has space left and
                 * realloc the maximum size.
                 */
                if (nitems >= arraysz) {
                        const int inc = 10;        /* increase by this much */
                        struct dirent **names2;

                        names2 = (struct dirent **)realloc((char *)names,
                                (arraysz + inc) * sizeof(struct dirent *));
                        if (names2 == NULL) {
                                free(p);
                                goto fail;
                        }
                        names = names2;
                        arraysz += inc;
                }
                names[nitems++] = p;
        }
        closedir(dirp);
        if (nitems && dcomp != NULL)
                qsort(names, nitems, sizeof(struct dirent *), dcomp);
        *namelist = names;
        return(nitems);

fail:
        while (nitems > 0)
                free(names[--nitems]);
        free(names);
        closedir(dirp);
        return -1;
}
#endif /* HAVE_SCANDIR */
#endif /* HAVE_DLFCN_H */

