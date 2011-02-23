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
 */
#ifndef _RPCMEM_H
#define _RPCMEM_H	1
/*
**
**  NAME:
**
**      rpcmem.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Various macros and data for runtime memory allocation.
**
**
*/

#include <dce/dce.h>

/*
 * Memory Allocation.
 *
 * The runtime dynamically allocates several classes of objects, each
 * with differing sizes and lifetimes.  There are a few structures that
 * are allocated in the "critical path" performance wise.  While the
 * runtime will likely be caching some of these structures, the performance
 * of the memory allocator should be of some concern.
 * 
 * Note that some structures are allocated by the listener task.  In
 * a kernel environment where the listener is implemented as a network
 * software interrupt handler, the memory allocators must be capable
 * of being called from an interrupt handler.  Additionally, the interface
 * we use allows one to specify blocking or non-blocking allocations,
 * non-blocking mode being required for interrupt level operations (which
 * obviously must be prepared to deal with a "no memory available"
 * indication).
 * 
 * We want to maintain a flexible interface to cope with the variety
 * of environments that this code may be ported to and to provide us
 * with hooks for determining how memory is being used.  The interface
 * we create and use is the same as that of OSF/1 NET_MALLOC (which is
 * like BSD4.4 kernel malloc).  We can always make RPC_MEM_ALLOC()
 * "smarter" and maintain a pool for various objects including a fast
 * allocation / free scheme layered on top of malloc(), etc.
 * 
 * If calling the allocator in a blocking mode (RPC_MEM_WAITOK), one
 * should be prepaired that the process may yield the processor (i.e.
 * make sure that you aren't violating the runtime's mutex lock rules).
 */


/*
 * Types of RPC memory objects that we allocate.
 */

#define RPC_C_MEM_AVAIL             0
#define RPC_C_MEM_DG_CHAND          1       /* rpc_dg_handle_client_t       */
#define RPC_C_MEM_DG_SHAND          2       /* rpc_dg_handle_server_t       */
#define RPC_C_MEM_DG_CCALL          3       /* rpc_dg_ccall_t               */
#define RPC_C_MEM_DG_SCALL          4       /* rpc_dg_scall_t               */
#define RPC_C_MEM_DG_PKT_POOL_ELT   5       /* rpc_dg_pkt_pool_t            */
#define RPC_C_MEM_DG_UNUSED         6       /*                              */
#define RPC_C_MEM_DG_RAWPKT         7       /* rpc_dg_raw_pkt_t             */
#define RPC_C_MEM_DG_PKTBODY        8       /* rpc_dg_pkt_body_t            */
#define RPC_C_MEM_DG_PKTLOG         9       /* from dg pktlog()             */
#define RPC_C_MEM_DG_CCTE          10       /* rpc_dg_cct_elt_t             */
#define RPC_C_MEM_DG_SCTE          11       /* rpc_dg_sct_elt_t             */
#define RPC_C_MEM_IF_RGY_ENTRY     12       /* rpc_if_rgy_entry_t           */
#define RPC_C_MEM_UTIL             13       /* from rpc_util_malloc()       */
#define RPC_C_MEM_SOCK_INFO        14       /* rpc_sock_info_t              */
#define RPC_C_MEM_IOVE_LIST        15       /* rpc_iove_list_elt_t          */
#define RPC_C_MEM_DG_LOSSY         16       /* misc stuff for dglossy.c     */
#define RPC_C_MEM_V1_PKTBODY       17
                                   /* pkt buff rpc__pre_v2_iface_server_call */
#define RPC_C_MEM_V1_STUB          18       /* rpc1_{alloc,free}_pkt()      */
#define RPC_C_MEM_IF_TYPE_INFO     19       /* rpc_if_type_info_t           */
#define RPC_C_MEM_LOOKASIDE_LIST   20       /* generic cn lookaside list elt */
#define RPC_C_MEM_RPC_ADDR         21       /* rpc_addr + naf data          */
#define RPC_C_MEM_DG_CLIENT_REP    22       /* rpc_dg_client_rep_t          */
#define RPC_C_MEM_DG_MAINT         23       /* maintain liveness structure  */
#define RPC_C_MEM_UUID             24       /* dce_uuid_t                       */
#define RPC_C_MEM_NSENTRY          25       /* rpc_nsentry_t                */
#define RPC_C_MEM_NSATTR           26       /* rpc_nsattr_t                 */
#define RPC_C_MEM_NSUUID           27       /* rpc_nsuuid_t                 */
#define RPC_C_MEM_TOWER_REF        28       /* rpc_tower_ref_t              */
#define RPC_C_MEM_TOWER            29       /* twr_t                        */
#define RPC_C_MEM_SYNTAX_ID        30       /* rpc_syntax_id_t              */
#define RPC_C_MEM_BINDING_VEC      31       /* rpc_binding_vector_t         */
#define RPC_C_MEM_CN_ASSOC_GRP_BLK 32       
                        /* rpc_assoc_group_t[rpc_c_assoc_grp_tbl_alloc_size] */
#define RPC_C_MEM_CN_ASSOC         33       /* rpc_assoc_t                  */
#define RPC_C_MEM_CN_CALL_REP      34       /* rpc_cn_call_rep_t            */
#define RPC_C_MEM_CN_LG_FRAGBUF    35       /* lg_fragbuf_alloc_size        */
#define RPC_C_MEM_CN_SM_FRAGBUF    36       /* sm_fragbuf_alloc_size        */
#define RPC_C_MEM_CN_BINDING_REP   37       /* rpc_cn_binding_rep_t         */
#define RPC_C_MEM_IMPORT           38       /* rpc_import_rep_t             */
#define RPC_C_MEM_IF_REP           39       /* rpc_if_rep_t                 */
#define RPC_C_MEM_LKUP_REP         40       /* rpc_lkup_rep_t               */
#define RPC_C_MEM_LKUP_NODE        41       /* rpc_lkup_node_t              */
#define RPC_C_MEM_LKUP_MBR         42       /* rpc_lkup_mbr_t               */
#define RPC_C_MEM_INQ_REP          43       /* rpc_inq_rep_t                */
#define RPC_C_MEM_NSPROFILE_ELT    44       /* rpc_nsprofile_elt_t          */
#define RPC_C_MEM_NSPROFILE_OCTET_STR 45    /* rpc_profile_elt_octet_string_t */
#define RPC_C_MEM_BINDING_VECTOR   46       /* rpc_binding_vector_t         */
#define RPC_C_MEM_CN_SYNTAX        47       /* rpc_syntax_t                 */
#define RPC_C_MEM_RPC_ADDR_VEC     48       /* rpc_addr + naf data          */
#define RPC_C_MEM_IF_ID            49       /* rpc_if_id_t                  */
#define RPC_C_MEM_IF_ID_VECTOR     50       /* rpc_if_id_vector_t           */
#define RPC_C_MEM_PROTSEQ_VECTOR   51       /* rpc_protseq_vector_t         */
#define RPC_C_MEM_STRING           52       /* various string types         */
#define RPC_C_MEM_MGR_EPV          53       /* manager epv                  */
#define RPC_C_MEM_EPT_ENTRY        54       /* endpoint mapper struct       */
#define RPC_C_MEM_UUID_VECTOR      55       /* uuid_vector_t                */
#define RPC_C_MEM_OBJ_RGY_ENTRY    56       /* rpc_obj_rgy_entry_t          */
#define RPC_C_MEM_THREAD_CONTEXT   57       /* rpc_thread_context_t         */
#define RPC_C_MEM_LISTENER_STATE   58       /* rpc_listener_state_t         */
#define RPC_C_MEM_NSRESOLUTION     59       /* portion of resolved name     */
#define RPC_C_MEM_NSRESIDUAL       60       /* portion of unresolved name   */
#define RPC_C_MEM_DG_SELACK_MASK   61       /* selective ack mask array     */
#define RPC_C_MEM_V1_HANDLE        62       /* pre-v2 binding handle        */
#define RPC_C_MEM_TOWER_FLOOR      63       /* rpc_tower_floor_t            */
#define RPC_C_MEM_TOWER_FLOOR_OCTET 64      /* rpc_tower_floor_t octet field */
#define RPC_C_MEM_TOWER_FLOOR_ID   65       /* rpc_tower_floor_t prot id field */
#define RPC_C_MEM_TOWER_PROT_IDS   66       /* rpc_tower_prot_ids_t         */
#define RPC_C_MEM_TOWER_REF_VECTOR 67       /* rpc_tower_ref_vector_t       */
#define RPC_C_MEM_SOCKADDR         68       /* sockaddr_p_t                 */
#define RPC_C_MEM_SOCKET_LIST      69       /* rpc_socket_t[*]              */
#define RPC_C_MEM_STATS_VECTOR     70       /* rpc_stats_vector             */
#define RPC_C_MEM_TOWER_VECTOR     71       /* rpc_tower_vector_t           */
#define RPC_C_MEM_DNA_TOWER        72       /* dna tower                    */
#define RPC_C_MEM_CN_SEC_CONTEXT   73       /* rpc_cn_sec_context_t         */
#define RPC_C_MEM_CTHREAD_POOL     74       /* cthread_pool_elt_t           */
#define RPC_C_MEM_CTHREAD_CTBL     75       /* cthread_elt_t[]              */
#define RPC_C_MEM_CTHREAD_QETBL    76       /* cthread_queue_elt_t[]        */
#define RPC_C_MEM_DG_SOCK_POOL_ELT 77       /* DG socket pool element       */
#define RPC_C_MEM_NOAUTH_INFO      78       /* rpc_noauth_info_t            */
#define RPC_C_MEM_NOAUTH_CN_INFO   79       /* rpc_noauth_cn_info_t         */
#define RPC_C_MEM_KRB_INFO         80       /* rpc_krb_info_t               */
#define RPC_C_MEM_KRB_CN_INFO      81       /* rpc_krb_cn_info_t            */
#define RPC_C_MEM_CN_ENCRYPT_BUF   82       
                                   /* buffer for encryption & checksumming */
#define RPC_C_MEM_PORT_RESTRICT_LIST 83     /* rpc_port_restriction_list_t  */
#define RPC_C_MEM_PORT_RANGE_ELEMENTS 84    /* rpc_port_range_element_t     */
#define RPC_C_MEM_CN_PAC_BUF       85       /* buffer for big PACs          */
#define RPC_C_MEM_FUNC             86       /* rpc_eval_func_t    */
#define RPC_C_MEM_EVAL             87       /* rpc_binding_eval_t    */
#define RPC_C_MEM_LIST             88       /* rpc_eval_list_t    */
#define RPC_C_MEM_DG_EPAC          89       /* used for oversized PACs      */
#define RPC_C_MEM_CDS_ATTR         90       /* attribute stored in CDS   */
#define RPC_C_MEM_PROTOCOL_VERSION 91       /* rpc_protocol_version_t */
#define RPC_C_MEM_NTLMSSPAUTH_INFO    92    /* rpc_ntlmsspauth_info_t       */
#define RPC_C_MEM_NTLMSSPAUTH_CN_INFO 93    /* rpc_ntlmsspauth_cn_info_t    */
#define RPC_C_MEM_GSSAUTH_INFO	94			/* rpc_gssauth_info_t */
#define RPC_C_MEM_GSSAUTH_CN_INFO	95		/* rpc_gssauth_cn_info_t */
#define RPC_C_MEM_NP_SEC_CONTEXT	96
#define RPC_C_MEM_NS_INFO 96                /* name services info err...    */
#define RPC_C_MEM_SCHNAUTH_INFO    98
#define RPC_C_MEM_SCHNAUTH_CN_INFO 99
#define RPC_C_MEM_NAMED_PIPE_INFO  100      /* rpc_np_auth_info_t */
#define RPC_C_MEM_NTLMAUTH_INFO    101      /* rpc_ntlmauth_info_t */
#define RPC_C_MEM_NTLMAUTH_CN_INFO 102      /* rpc_ntlmauth_cn_info_t */

/* can only use up to "rpc_c_mem_maxtypes - 1" without upping it */
#define RPC_C_MEM_MAX_TYPES        103       /* i.e. 0 : (max_types - 1)     */


/*
 * RPC memory use statistics database and database mutex.
 * This is a LEVEL 3 mutex.
 *
 * See rpcglob.[ch] for the actual database and lock.
 */

typedef struct 
{
    unsigned32 inuse;         /* number currently allocated */
    unsigned32 calls;         /* total ever allocated */
    unsigned32 fails;         /* denied alloc requests */
    unsigned32 maxsize;       /* max size allocated for this type */
} rpc_mem_stats_elt_t, *rpc_mem_stats_elt_p_t;

EXTERNAL rpc_mem_stats_elt_t rpc_g_mem_stats[];

/*
 * Values for the 'flags' argument to RPC_MEM_ALLOC
 */

#define RPC_C_MEM_WAITOK    0
#define RPC_C_MEM_NOWAIT    1


/*
 * Concurrency control for the mem statistics database.
 * We avoid the locking for the moment... after all, this is
 * only statistics stuff.
 */

#ifdef RPC_STATISTICS_LOCK	/* ??? */
#define RPC_MEM_LOCK_INIT(junk)         RPC_MUTEX_INIT(rpc_g_global_mutex)
#define RPC_MEM_LOCK(junk)              RPC_MUTEX_LOCK(rpc_g_global_mutex)
#define RPC_MEM_UNLOCK(junk)            RPC_MUTEX_UNLOCK(rpc_g_global_mutex)
#define RPC_MEM_TRY_LOCK(bp)            RPC_MUTEX_TRY_LOCK(rpc_g_global_mutex,(bp))
#define RPC_MEM_LOCK_DELETE(junk)       RPC_MUTEX_DELETE(rpc_g_global_mutex)
#define RPC_MEM_LOCK_ASSERT(junk)       RPC_MUTEX_LOCK_ASSERT(rpc_g_global_mutex)
#define RPC_MEM_UNLOCK_ASSERT(junk)     RPC_MUTEX_UNLOCKED_ASSERT(rpc_g_global_mutex)
#else
#define RPC_MEM_LOCK_INIT(junk)
#define RPC_MEM_LOCK(junk)
#define RPC_MEM_UNLOCK(junk)
#define RPC_MEM_TRY_LOCK(bp)
#define RPC_MEM_LOCK_DELETE(junk)
#define RPC_MEM_LOCK_ASSERT(junk)
#define RPC_MEM_UNLOCK_ASSERT(junk)
#endif /* RPC_STATISTICS_LOCK */


/*
 * Map the RPC_MEM_ operations to either INLINE or Out-of-line
 * implementations.  The default is Out-of-line, but this can be
 * changed via the system specific configuration file.
 */

#ifdef RPC_MEM_DEFAULT_INLINE
#  define RPC_MEM_ALLOC(addr, cast, size, type, flags) \
        RPC_MEM_ALLOC_IL(addr, cast, size, type, flags)

#  define RPC_MEM_REALLOC(addr, cast, size, type, flags) \
        RPC_MEM_REALLOC_IL(addr, cast, size, type, flags)

#  define RPC_MEM_FREE(addr, type) \
        RPC_MEM_FREE_IL(addr, type)
#else
#  define RPC_MEM_ALLOC(addr, cast, size, type, flags) \
        (addr) = (cast) rpc__mem_alloc(size, type, flags)

#  define RPC_MEM_REALLOC(addr, cast, size, type, flags) \
        (addr) = (cast) rpc__mem_realloc(addr, size, type, flags)

#  define RPC_MEM_FREE(addr, type) \
        rpc__mem_free((pointer_t)(addr), type)
#endif


/*
 * R P C _ M E M _ A L L O C _ I L
 * 
 * (addr) == NULL iff "no memory available"
 *
 * Sample usage:
 *      rpc_dg_ccall_p_t ccall;
 *      RPC_MEM_ALLOC(ccall, rpc_dg_ccall_p_t, sizeof *rpc_dg_ccall_p_t, 
 *              rpc_c_mem_dg_ccall, rpc_c_mem_nowait);
 *      if (ccall == NULL)
 *          alloc failed
 *
 * Note that we just raise an exception if the malloc fails (since most
 * callers don't yet check the return value).  In any case, this will
 * probably always be the correct thing to do in a user space NCK
 * implementation).
 */

#define RPC_MEM_ALLOC_IL(addr, cast, size, type, flags) \
{ \
    RPC_LOG_MEM_ALLOC_NTR; \
    (addr) = (cast) malloc(size); \
    RPC_MEM_LOCK (0); \
    rpc_g_mem_stats[type].calls++; \
    if ((addr) == NULL) { \
        rpc_g_mem_stats[type].fails++; \
	RPC_DCE_SVC_PRINTF (( \
	    DCE_SVC(RPC__SVC_HANDLE, "%s"), \
	    rpc_svc_mem, \
	    svc_c_sev_fatal | svc_c_action_abort, \
	    rpc_m_alloc_fail, \
	    "RPC_MEM_ALLOC" )); \
    } else \
        rpc_g_mem_stats[type].inuse++; \
    if ((size) > rpc_g_mem_stats[type].maxsize) \
        rpc_g_mem_stats[type].maxsize = (size); \
    RPC_MEM_UNLOCK (0); \
    RPC_LOG_MEM_ALLOC_XIT; \
}


/*
 * R P C _ M E M _ R E A L L O C _ I L
 * 
 * (addr) == NULL iff "no memory available"
 *
 * Sample usage:
 *      rpc_dg_ccall_p_t ccall;
 *      RPC_MEM_REALLOC(ccall, rpc_dg_ccall_p_t, sizeof *rpc_dg_ccall_p_t, 
 *              rpc_c_mem_dg_ccall, rpc_c_mem_nowait);
 *      if (ccall == NULL)
 *          alloc failed
 *
 * Note that we just raise an exception if the realloc fails (since most
 * callers don't yet check the return value).  In any case, this will
 * probably always be the correct thing to do in a user space NCK
 * implementation).
 */

#define RPC_MEM_REALLOC_IL(addr, cast, size, type, flags) \
{ \
    RPC_LOG_MEM_REALLOC_NTR; \
    (addr) = (cast) realloc(addr, size); \
    RPC_MEM_LOCK (0); \
    rpc_g_mem_stats[type].calls++; \
    if ((addr) == NULL) { \
        rpc_g_mem_stats[type].fails++; \
	RPC_DCE_SVC_PRINTF (( \
	    DCE_SVC(RPC__SVC_HANDLE, "%s"), \
	    rpc_svc_mem, \
	    svc_c_sev_fatal | svc_c_action_abort, \
	    rpc_m_realloc_fail, \
	    "RPC_MEM_REALLOC" )); \
    } else \
        rpc_g_mem_stats[type].inuse++; \
    if ((size) > rpc_g_mem_stats[type].maxsize) \
        rpc_g_mem_stats[type].maxsize = (size); \
    RPC_MEM_UNLOCK (0); \
    RPC_LOG_MEM_REALLOC_XIT; \
}


/*
 * R P C _ M E M _ F R E E _ I L
 *
 * Sample useage:
 *      ...
 *      RPC_MEM_FREE(ccall, rpc_c_mem_dg_ccall);
 */

#define RPC_MEM_FREE_IL(addr, type) \
{ \
    RPC_LOG_MEM_FREE_NTR; \
    free(((char *) (addr))); \
    RPC_MEM_LOCK (0); \
    --rpc_g_mem_stats[type].inuse; \
    RPC_MEM_UNLOCK (0); \
    RPC_LOG_MEM_FREE_XIT; \
}


PRIVATE pointer_t rpc__mem_alloc (
        unsigned32 /*size*/,
        unsigned32 /*type*/,
        unsigned32  /*flags*/
    );

PRIVATE pointer_t rpc__mem_realloc (
        pointer_t  /*addr*/,
        unsigned32 /*size*/,
        unsigned32 /*type*/,
        unsigned32  /*flags*/
    );

PRIVATE void rpc__mem_free (
        pointer_t   /*addr*/,
        unsigned32  /*type*/
    );

#endif /* _RPCMEM_H */
