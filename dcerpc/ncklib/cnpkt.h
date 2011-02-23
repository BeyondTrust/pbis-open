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
#ifndef _CNPKT_H
#define _CNPKT_H

/*
**
**  NAME
**
**      cnpkt.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Definitions of types/constants for connection-based protocol packets.
**
**
*/

#ifdef DEBUG
#define RPC_CN_PKT_TRC(pkt)\
{\
    RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT,\
                    ("PACKET: type->%s flags->%x drep->%02x%02x%02x%02x frag_len->%d auth_len->%d call_id->%x\n",\
                     rpc__cn_pkt_name(RPC_CN_PKT_PTYPE (pkt)),\
                     RPC_CN_PKT_FLAGS (pkt),\
                     RPC_CN_PKT_DREP (pkt)[0], \
                     RPC_CN_PKT_DREP (pkt)[1], \
                     RPC_CN_PKT_DREP (pkt)[2], \
                     RPC_CN_PKT_DREP (pkt)[3], \
                     RPC_CN_PKT_FRAG_LEN (pkt),\
                     RPC_CN_PKT_AUTH_LEN (pkt),\
                     RPC_CN_PKT_CALL_ID (pkt)));\
}
#else
#define RPC_CN_PKT_TRC(pkt)
#endif

#ifdef DEBUG
#define RPC_CN_IOV_DUMP(iov)\
{\
    unsigned16  k_, acc_;\
    for (acc_ = 0, k_ = 0; k_ < (iov)->num_elt; k_++)\
    {\
         RPC_DBG_PRINTF (rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT_DUMP,\
                         ("PACKET: fragment->#%d addr->%x\n", k_, (iov)->elt[k_].data_addr));\
         RPC_CN_MEM_DUMP ((iov)->elt[k_].data_addr, (iov)->elt[k_].data_len, acc_);\
    }\
}
#else
#define RPC_CN_IOV_DUMP(iov)
#endif

#ifdef DEBUG
#define RPC_CN_PKT_DUMP(addr, len)\
{\
    unsigned16  acc_;\
    acc_ = 0;\
    RPC_CN_MEM_DUMP (addr, len, acc_);\
}
#else
#define RPC_CN_PKT_DUMP(addr, len)
#endif

#ifdef DEBUG
#define RPC_CN_MEM_DUMP(addr, len, acc)\
{\
    if (RPC_DBG2(rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT_DUMP))\
    {\
        unsigned16  i_, j_;\
        char  *p_, buff_[80];\
        for (i_ = 0; i_ < (len);)\
        {\
            p_ = buff_;\
            sprintf(p_, "PACKET: <%04d> ", (acc));\
            p_ = buff_ + strlen(buff_);\
            for (j_ = 0; (i_ < (len)) && (j_ < 8); j_++)\
            {\
                sprintf(p_, "%02x%02x ",\
                                 (unsigned8)(((unsigned8 *)(addr))[i_]),\
                                 (unsigned8)(((unsigned8 *)(addr))[i_ + 1]));\
                p_ = buff_ + strlen(buff_);\
                i_ += 2;\
                (acc) += 2;\
            }\
            sprintf(p_, "\n");\
            RPC_DBG_PRINTF(rpc_e_dbg_cn_pkt, RPC_C_CN_DBG_PKT_DUMP, (buff_));\
        }\
    }\
}
#else
#define RPC_CN_MEM_DUMP(addr, len, acc)
#endif


/*
 *****************************************************************************
 *
 * A macro for doing endian conversions for connection protocol syntax.
 *
 *****************************************************************************
 */

#define SWAB_INPLACE_SYNTAX(sfield) { \
    SWAB_INPLACE_UUID( (sfield).id ); \
    SWAB_INPLACE_32( (sfield).version ); \
}


/*
 *****************************************************************************
 *
 * Some basic definitions used in defining packets
 *
 *****************************************************************************
 */

/*
 * This is the version number contained in the packet header.
 */
#define RPC_C_CN_PROTO_VERS       5       /* RunTime protocol version */
#define RPC_C_CN_PROTO_VERS_MINOR 1

/*
 * This is the compat minor version for new client to old server
 */
#define RPC_C_CN_PROTO_VERS_COMPAT 0

/*
 * This is the architecture protocol id (for storage in protocol towers).
 */

#define RPC_C_CN_PROTO_ID  0x0B

/*
 *****************************************************************************
 *
 * These are all the packet types which can be sent over the wire in the
 * connection based Request/Response (RR) protocol.
 */
#define RPC_C_CN_PKT_REQUEST               0    /* client -> server */
#define RPC_C_CN_PKT_PING                  1    /* client -> server */
#define RPC_C_CN_PKT_RESPONSE              2    /* server -> client */
#define RPC_C_CN_PKT_FAULT                 3    /* server -> client */
#define RPC_C_CN_PKT_WORKING               4    /* server -> client */
#define RPC_C_CN_PKT_NOCALL                5    /* server -> client */
#define RPC_C_CN_PKT_REJECT                6    /* server -> client */
#define RPC_C_CN_PKT_ACK                   7    /* client -> server */
#define RPC_C_CN_PKT_QUIT                  8    /* client -> server */
#define RPC_C_CN_PKT_FACK                  9    /* both directions  */
#define RPC_C_CN_PKT_QUACK                10    /* server -> client */
#define RPC_C_CN_PKT_BIND                 11    /* client -> server */
#define RPC_C_CN_PKT_BIND_ACK             12    /* server -> client */
#define RPC_C_CN_PKT_BIND_NAK             13    /* server -> client */
#define RPC_C_CN_PKT_ALTER_CONTEXT        14    /* client -> server */
#define RPC_C_CN_PKT_ALTER_CONTEXT_RESP   15    /* server -> client */
#define RPC_C_CN_PKT_AUTH3                16    /* client -> server */
#define RPC_C_CN_PKT_SHUTDOWN             17    /* server -> client */
#define RPC_C_CN_PKT_REMOTE_ALERT         18    /* client -> server */
#define RPC_C_CN_PKT_ORPHANED             19    /* client -> server */
#define RPC_C_CN_PKT_MAX_TYPE             19
#define RPC_C_CN_PKT_INVALID              0xff 

/*
 * Values for the flag field in the packet header.
 */
#define RPC_C_CN_FLAGS_FIRST_FRAG       0x01    /* First fragment */
#define RPC_C_CN_FLAGS_LAST_FRAG        0x02    /* Last fragment */
#define RPC_C_CN_FLAGS_ALERT_PENDING    0x04    /* Alert was pending at sender */
#define RPC_C_CN_FLAGS_RESERVED_1       0x08    /* Reserved, m.b.z. */
#define RPC_C_CN_FLAGS_CONCURRENT_MPX   0x10    /* Supports concurrent multiplexing
                                                 * of a single connection */
#define RPC_C_CN_FLAGS_DID_NOT_EXECUTE  0x20    /* only meaningful on `fault'
                                                 * packet; if true, call did
                                                 * not execute. */
#define RPC_C_CN_FLAGS_MAYBE            0x40    /* 'maybe' semantics requested */
#define RPC_C_CN_FLAGS_OBJECT_UUID      0x80    /* if true, a non-nil object UUID
                                                 * was specified in the handle,
                                                 * and is present in the optional
                                                 * object field. If false, the
                                                 * object field is omitted */


/*
 *****************************************************************************
 *
 * Some basic typedefs used in defining packets
 *
 *****************************************************************************
 */

/*
 * This is the format of the optional data sent on a connect disconnect, or
 * connect reject message.
 */
#define RPC_C_CN_OPTIONAL_DATA    16
typedef unsigned16 rpc_cn_reason_code_t;
typedef struct
{
    unsigned8   rpc_vers;               /* rpc_c_cn_proto_vers */
    unsigned8   rpc_vers_minor;         /* rpc_c_cn_proto_vers_minor */
    unsigned8   reserved1[2];           /* must be zero */
    unsigned8   drep[4];
    unsigned32  reject_status;
    unsigned8   reserved2[4];
} rpc_cn_optional_data_t, *rpc_cn_optional_data_p_t;

typedef struct
{
    rpc_cn_reason_code_t        reason_code;
    rpc_cn_optional_data_t      rpc_info;   /* may be RPC specific */
} rpc_cn_reject_optional_data_t, *rpc_cn_reject_optional_data_p_t;

typedef struct
{
    rpc_cn_reason_code_t        reason_code;
    rpc_cn_optional_data_t      rpc_info;   /* may be RPC specific */
} rpc_cn_disc_optional_data_t, *rpc_cn_disc_optional_data_p_t;

/*
 ****************************************************************************
 *
 * R P C _ C N _ P O R T _ A N Y _ T
 *
 * A counted string that can contain an address from any address family.
 *
 * The local part of the address, any address family, string rep.
 *
 * Note: Include C null termination in the count and null
 *       terminate the value.
 */
typedef struct
{
    unsigned16 length;                 /* length of the array */
    unsigned8  s[1];                   /* minimally, an array of 1 character */
} rpc_cn_port_any_t, *rpc_cn_port_any_p_t;



/*
 *****************************************************************************
 *
 * Presentation negotiation structure definitions.
 *
 * These structures are used in the 'bind' and 'bind_ack' packets.
 *
 *****************************************************************************
 */

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ C O N T E X T _ I D _ T
 *
 * This is the presentation context identifier.
 */
typedef unsigned16 rpc_cn_pres_context_id_t;

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ S Y N T A X _ I D _ T
 *
 * This is a presentation syntax identifier. Used for abstract syntax with id set
 * to the interface UUID and version set to the interface version.
 * Also used for the transfer syntax, with a well-known UUID and version of the data
 * rep. The major version is encoded in the 16LSBits of if_version,
 * and the minor version is encoded in the 16MSBits of if_version.
 */
typedef struct
{
    dce_uuid_t     id;
    unsigned32 version;
} rpc_cn_pres_syntax_id_t, *rpc_cn_pres_syntax_id_p_t;

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ C O N T _ E L E M _ T
 *
 * This is a presentation syntax negotiation context element.
 */
typedef struct
{
    rpc_cn_pres_context_id_t pres_context_id;
    unsigned8               n_transfer_syn;       /* number of items */
    unsigned8               reserved;             /* alignment pad, m.b.z. */
    rpc_cn_pres_syntax_id_t abstract_syntax;
    rpc_cn_pres_syntax_id_t transfer_syntaxes[1]; /* [size_is(n_transfer_syn)] */
} rpc_cn_pres_cont_elem_t, *rpc_cn_pres_cont_elem_p_t;

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ C O N T _ L I S T _ T
 *
 * This is a list of presentation syntax negotiation context elements.
 */
typedef struct
{
    unsigned8               n_context_elem;       /* number of items */
    unsigned8               reserved;             /* alignment pad, m.b.z. */
    unsigned16              reserved2;            /* alignment pad, m.b.z. */
    rpc_cn_pres_cont_elem_t pres_cont_elem[1];    /* [size_is(n_context_elem)] */
} rpc_cn_pres_cont_list_t, *rpc_cn_pres_cont_list_p_t;

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ C O N T _ D E F _ R E S U L T _ T
 *
 * Presentation syntax negotiation result type.
 */
typedef unsigned16 rpc_cn_pres_cont_def_result_t;

#define RPC_C_CN_PCONT_ACCEPTANCE          0
#define RPC_C_CN_PCONT_USER_REJECTION      1
#define RPC_C_CN_PCONT_PROVIDER_REJECTION  2

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ P R O V I D E R _ R E A S O N _ T
 *
 * Presentation syntax negotiation provider reason code.
 */
typedef unsigned16 rpc_cn_pres_provider_reason_t;

#define RPC_C_CN_PPROV_REASON_NOT_SPECIFIED                      0
#define RPC_C_CN_PPROV_ABSTRACT_SYNTAX_NOT_SUPPORTED             1
#define RPC_C_CN_PPROV_PROPOSED_TRANSFER_SYNTAXES_NOT_SUPPORTED  2
#define RPC_C_CN_PPROV_LOCAL_LIMIT_EXCEEDED                      3

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ R E J E C T _ R E A S O N _ T
 *
 * Presentation syntax negotiation reject reason code.
 */
typedef unsigned16 rpc_cn_pres_reject_reason_t;

#define RPC_C_CN_PREJ_REASON_NOT_SPECIFIED            0
#define RPC_C_CN_PREJ_TEMPORARY_CONGESTION            1
#define RPC_C_CN_PREJ_LOCAL_LIMIT_EXCEEDED            2
#define RPC_C_CN_PREJ_CALLED_PADDR_UNKNOWN            3 /* not used */
#define RPC_C_CN_PREJ_PROTOCOL_VERSION_NOT_SUPPORTED  4
#define RPC_C_CN_PREJ_DEFAULT_CONTEXT_NOT_SUPPORTED   5 /* not used */
#define RPC_C_CN_PREJ_USER_DATA_NOT_READABLE          6 /* not used */
#define RPC_C_CN_PREJ_NO_PSAP_AVAILABLE               7 /* not used */

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ R E S U L T _ T
 *
 * Presentation syntax negotiation context result element.
 */
typedef struct
{
    rpc_cn_pres_cont_def_result_t result;
    rpc_cn_pres_provider_reason_t reason;          /* only if not ok */
    rpc_cn_pres_syntax_id_t       transfer_syntax; /* transfer syntax
                                                    * selected 0 if result not
                                                    * accepted */
} rpc_cn_pres_result_t, *rpc_cn_pres_result_p_t;

/*
 *****************************************************************************
 *
 * R P C _ C N _ P R E S _ R E S U L T _ L I S T _ T
 *
 * The presentation syntax negotiation context definition result list, same
 * order and num of elements as in the rpc_pres_cont_list_t in the
 * bind request.
 */
typedef struct
{
    unsigned8           n_results;              /* count */
    unsigned8           reserved;               /* alignment pad, m.b.z. */
    unsigned16          reserved2;              /* alignment pad, m.b.z. */
    rpc_cn_pres_result_t pres_results[1];       /* [size_is(n_results)] */
} rpc_cn_pres_result_list_t, *rpc_cn_pres_result_list_p_t;




/*
 *****************************************************************************
 *
 * Version datatypes
 *
 * These structures are used in the 'bind_nack' packet.
 *
 *****************************************************************************
 */

/*
 *****************************************************************************
 *
 * R P C _ C N _ V E R S I O N _ T
 *
 * This structure is the basic version type.
 */
typedef struct
{
    unsigned8 vers_major;
    unsigned8 vers_minor;
} rpc_cn_version_t, *rpc_cn_version_p_t;
 
/*
 *****************************************************************************
 *
 * R P C _ C N _ V E R S I O N S _ S U P P O R T E D _ T
 *
 * This structure contains the list of protocol versions supported and is
 * returned in the 'bind_nak' packet.
 */
typedef struct
{
    unsigned8           n_protocols;    /* count */
    rpc_cn_version_t    protocols[1];   /* [max_is(n_protocols)] */
} rpc_cn_versions_supported_t, *rpc_cn_versions_supported_p_t;
 


/*
 *****************************************************************************
 *
 * Some common header structures
 *
 *****************************************************************************
 */

/*
 *****************************************************************************
 *
 * R P C _ C N _ C O M M O N _ H D R _ T
 *
 * This is the common part of all packet headers.
 */
typedef struct
{
    unsigned8  rpc_vers;               /* 00:01 RPC version - major */
    unsigned8  rpc_vers_minor;         /* 01:01 RPC version - minor */
    unsigned8  ptype;                  /* 02:01 packet type */
    unsigned8  flags;                  /* 03:01 flags */
    unsigned8  drep[4];                /* 04:04 ndr format */
    unsigned16 frag_len;               /* 08:02 fragment length */
    unsigned16 auth_len;               /* 10:02 authentication length */
    unsigned32 call_id;                /* 12:04 call identifier */
} rpc_cn_common_hdr_t, *rpc_cn_common_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_COMMON_HDR    16

/*
 *
 * The following define specifies the number of bytes in the header that
 * must be read in order to get at the frag_len field of the header.  We
 * need this to determine the number of bytes in the fragment while we are
 * "receiving" packets.
 */

#define RPC_C_CN_FRAGLEN_HEADER_BYTES 10


/*
 *****************************************************************************
 *
 * Authentication
 *
 *****************************************************************************
 */
/*
 * Determine whether a PDU contains an authentication trailer.
 */
#define RPC_CN_PKT_AUTH_TLR_PRESENT(pkt_p) (RPC_CN_PKT_AUTH_LEN(pkt_p) != 0)

/*
 * Return the size of the authentication trailer, if any, on a PDU.
 */
#define RPC_CN_PKT_AUTH_TLR_LEN(pkt_p) \
(RPC_CN_PKT_AUTH_TLR_PRESENT(pkt_p) ? (RPC_CN_PKT_AUTH_LEN(pkt_p)  +\
                                       RPC_CN_PKT_SIZEOF_COM_AUTH_TLR) : 0)

/*
 * Return the size of the padding added to align the authentication
 * trailer on a 4-byte boundary.
 */
#define RPC_CN_PKT_STUB_DATA_PAD_LEN(pkt_p, pkt_len)\
(RPC_CN_PKT_AUTH_TLR_PRESENT(pkt_p) ? ((RPC_CN_PKT_AUTH_TLR(pkt_p, pkt_len))->stub_pad_length) : 0)
    
/*
 * Return a pointer to the authentication trailer on a PDU. This
 * macros assumes there is an authentication trailer on the PDU.
 */
#define RPC_CN_PKT_AUTH_TLR(pkt_p, pkt_len)\
    (rpc_cn_auth_tlr_t *) ((unsigned_char_p_t)(pkt_p) + pkt_len - RPC_CN_PKT_AUTH_TLR_LEN(pkt_p))

/*
 *****************************************************************************
 *
 * R P C _ C N _ A U T H _ T L R _ T
 *
 * This is the authentication trailer template.
 *
 * Note: The authentication trailer is only present if auth_length
 *       in common part of header is non-zero.
 *       The authentication trailer is variable size.
 *       The authentication trailer is aligned on a 4-byte boundary.
 *       The encodings of the auth_value field is authentication
 *       service specific.
 */
typedef struct
{
    unsigned8 auth_type;               /* :01 which authent service */
    unsigned8 auth_level;              /* :01 which level within service */
    unsigned8 stub_pad_length;         /* :01 length of stub padding */
    unsigned8 reserved;                /* :01 alignment pad m.b.z. */
    unsigned32  key_id;                /* :04 key ID */
    unsigned8 auth_value[1];           /* :yy [size_is (auth_length)] credentials */
    /* No trailing alignment needed here */
} rpc_cn_auth_tlr_t, *rpc_cn_auth_tlr_p_t;

#define RPC_CN_PKT_SIZEOF_COM_AUTH_TLR  8

/* 
 * The valid values for the auth_type field are contained in nbase.idl.
 */

/* 
 * The valid values for the auth_level field are contained in nbase.idl.
 */

/*
 *****************************************************************************
 *
 * R P C _ C N _ B I N D _ A U T H _ V A L U E _ P R I V _ T
 *
 * This is the authentication trailer auth_value field layout for
 * bind and alter_context PDUs for the OSF DCE Private-key
 * authentication protocol.
 */
#define RPC_C_CN_DCE_SUB_TYPE     0
#define RPC_C_CN_DCE_SUB_TYPE_MD5 1
#define RPC_C_CN_DCE_CKSM_LEN     8
#define RPC_C_CN_DCE_MD5_CKSM_LEN 16

typedef struct
{
    unsigned32  assoc_uuid_crc;
    unsigned8   sub_type;
    unsigned8   checksum_length;
    unsigned16  cred_length;
    unsigned8   credentials[1];         /* [size_is (cred_length)] */
/*  unsigned8   checksum[1];  */        /* [size_is (checksum_length)] */
} rpc_cn_bind_auth_value_priv_t, *rpc_cn_bind_auth_value_priv_p_t;

#define RPC_CN_PKT_SIZEOF_BIND_AUTH_VAL         8

/*
 *****************************************************************************
 *
 * R P C _ C N _ A U T H _ V A L U E _ P R I V _ T
 *
 * This is the authentication trailer auth_value field layout for
 * PDUs other than bind and alter_context PDUs for the OSF DCE
 * Private-key authentication protocol. Note that the
 * bind_nak PDU cannot contain an authentication trailer.
 */
typedef struct
{
    unsigned8   sub_type;
/* 
 * only if auth_level pkt or higher 
 */
    unsigned8   checksum_length;
    unsigned8   checksum[1];    /* [size_is (checksum_length)] */
} rpc_cn_auth_value_priv_t, *rpc_cn_auth_value_priv_p_t;

#define RPC_CN_PKT_SIZEOF_AUTH_VAL              1
#define RPC_CN_PKT_SIZEOF_AUTH_VAL_CKSM         2



/*
 *****************************************************************************
 *
 * Now, what you've all been waiting for:
 *
 * The connection based RPC packet definitions.
 *
 *****************************************************************************
 */

/*
 *****************************************************************************
 *
 * R P C _ C N _ B I N D _ H D R _ T
 *
 * This is the rpc_bind packet header template.
 */
typedef union
{
    double force_alignment;            /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr;/* 00:16 common to all packets */
        unsigned16 max_xmit_frag;      /* 16:02 max transmit frag size, kb. */
        unsigned16 max_recv_frag;      /* 18:02 max receive  frag size, kb. */
        unsigned32 assoc_group_id;     /* 20:04 incarnation of client-server
                                        * group */
                                       /* 24:xx presentation context list */
        /* rpc_cn_pres_cont_list_t pres_context_list; */
        
        /* restore 4-byte alignment */
        /* rpc_cn_auth_tlr_t auth_tlr; */ /* if auth_len != 0 */
    } hdr;
} rpc_cn_bind_hdr_t, *rpc_cn_bind_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_BIND_HDR      (RPC_CN_PKT_SIZEOF_COMMON_HDR + 2 + 2 + 4)



/*
 *****************************************************************************
 *
 * R P C _ C N _ B I N D _ A C K _ H D R _ T
 *
 * This is the rpc_bind_ack packet header template.
 */

typedef union
{
    double force_alignment;            /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr;/* 00:16 common to all packets */
        unsigned16 max_xmit_frag;      /* 16:02 max transmit frag size, kb. */
        unsigned16 max_recv_frag;      /* 18:02 max receive  frag size, kb. */
        unsigned32 assoc_group_id;     /* 20:04 incarnation of client-server
                                        * group */
        /* rpc_cn_port_any_t sec_addr;*//* 24:xx optional secondary address for
                                        * process incarnation; local port part
                                        * of address only */

        /* restore 4-byte alignment */
        /* rpc_cn_pres_result_list_t pres_result_list; xx:xx presentation result */

        /* restore 4-byte alignment */
        /* rpc_cn_auth_tlr_t auth_tlr;  if auth_len != 0 */
    } hdr;
} rpc_cn_bind_ack_hdr_t, *rpc_cn_bind_ack_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_BIND_ACK_HDR (RPC_CN_PKT_SIZEOF_COMMON_HDR + 2 + 2 + 4)



/*
 *****************************************************************************
 *
 * R P C _ C N _ B I N D _ N A C K _ H D R _ T
 *
 * This is the rpc_bind_nack packet header template.
 */
typedef union
{
    double force_alignment;             /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr; /* 00:16 common to all packets */
                                        /* 16:02 pres conn reject reason */
        rpc_cn_pres_reject_reason_t provider_reject_reason;
                                        /* 18:yy array of protocol versions supported */
        rpc_cn_versions_supported_t versions;
    } hdr;
} rpc_cn_bind_nack_hdr_t, *rpc_cn_bind_nack_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_BIND_NACK_HDR         (RPC_CN_PKT_SIZEOF_COMMON_HDR + 2 + 1 + 2)


/*
 *****************************************************************************
 *
 * R P C _ C N _ A U T H 3 _ H D R _ T
 *
 * This is the rpc_auth3 packet header template.
 *
 * lkcl: NT has the max_xmit_frag and max_recv_frag here...
 */
typedef union
{
    double force_alignment;             /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr; /* 00:16 common to all packets */
        unsigned16 max_xmit_frag;       /* 16:02 max transmit frag size */
        unsigned16 max_recv_frag;       /* 18:02 max receive  frag size */
	/*
        rpc_cn_auth_tlr_t auth_tlr;
	*/
    } hdr;
} rpc_cn_auth3_hdr_t, *rpc_cn_auth3_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_AUTH3_HDR     (RPC_CN_PKT_SIZEOF_COMMON_HDR + 2 + 2)



/*
 *****************************************************************************
 *
 * R P C _ C N _ R E Q U E S T _ H D R _ T
 *
 * This is the rpc_request packet header template.
 */

/*
 * We define a union for the object UUID field with the start of
 * the stub data (if there is no UUID field present).
 */
typedef union
{
    dce_uuid_t object;                     /* 24:16 object UID */
    unsigned8 stub_data_no_object[1];  /* stub data */
} object_or_stub_data_t;

/*
 * Note that we define the stub data to have 8 bytes so that it's
 * 8 byte aligned.
 */
typedef unsigned8 stub_data_t[8];

typedef union
{
    double force_alignment;            /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr;/* 00:16 common to all packets  */

        unsigned32 alloc_hint;         /* 16:04 allocation hint */
        rpc_cn_pres_context_id_t pres_cont_id;        
                                       /* 20:02 pres context, ie. drep */

        unsigned16 opnum;              /* 22:02 operation # w/in i/f */

        /*
         * optional field for request, only present if the FLG_OBJECT_UUID
         * flags bit is set
         */
        object_or_stub_data_t object;  /* 24:16 object UID */

        /*
         * if the object uuid field is present, then the
         * stub data goes here, 8-byte aligned
         */
        stub_data_t stub_data_w_object;

        /* restore 4-byte alignment */
        /* rpc_cn_auth_tlr_t     auth_tlr; */  /* if auth_len != 0 */
    } hdr;
} rpc_cn_request_hdr_t, *rpc_cn_request_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_RQST_HDR      (RPC_CN_PKT_SIZEOF_COMMON_HDR + 4 + 2 + 2)

/*
 * Macro to determine whether the object UUID field is present.
 */

#define RPC_CN_PKT_OBJ_UUID_PRESENT(pkt_p)\
    (RPC_CN_PKT_FLAGS (pkt_p) & RPC_C_CN_FLAGS_OBJECT_UUID)

/*
 * Size of the request header, excluding stub data, with and
 * without the object uuid.
 */
#define RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ \
    RPC_CN_PKT_SIZEOF_RQST_HDR
#define RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ \
    (RPC_CN_PKT_SIZEOF_RQST_HDR + sizeof (dce_uuid_t))



/*
 *****************************************************************************
 *
 * R P C _ C N _ R E S P O N S E _ H D R _ T
 *
 * This is the rpc_response packet header template.
 */
typedef union
{
    double force_alignment;            /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr;/* 00:16 common to all packets */

        unsigned32 alloc_hint;         /* 16:04 allocation hint */
                                       /* 20:02 pres context */
        rpc_cn_pres_context_id_t pres_cont_id;        
        unsigned8  alert_count;        /* 22:01 pending alert count */
        unsigned8  reserved;           /* 23:01 alignment pad m.b.z. */
        stub_data_t stub_data;         /* 24:yy stub data */

        /* restore 4-byte alignment */
        /* rpc_cn_auth_tlr_t     auth_tlr; */  /* if auth_len != 0 */
    } hdr;
} rpc_cn_response_hdr_t, *rpc_cn_response_hdr_p_t;

/*
 * Size of the response header, excluding stub data.
 */

#define RPC_CN_PKT_SIZEOF_RESP_HDR      (RPC_CN_PKT_SIZEOF_COMMON_HDR + 4 + 2 + 1 + 1)


/*
 *****************************************************************************
 *
 * R P C _ C N _ F A U L T _ H D R _ T
 *
 * This is the rpc_fault packet header template.
 */
typedef union
{
    double force_alignment;            /* Get highest alignment possible */
    struct
    {
        rpc_cn_common_hdr_t common_hdr;/* 00:16 common to all packets */

        unsigned32 alloc_hint;         /* 16:04 allocation hint */
                                       /* 20:02 pres context, ie. drep */
        rpc_cn_pres_context_id_t pres_cont_id;        
        unsigned8  alert_count;        /* 22:01 pending alert count */
        unsigned8  reserved;           /* 23:01 alignment pad */
        unsigned32 status;             /* 24:04 runtime fault code or zero */
        unsigned32 reserved2;          /* 28:04 alignment pad m.b.z. */
        stub_data_t stub_data;         /* 32:yy stub data */

        /* restore 4-byte alignment */
        /* rpc_cn_auth_tlr_t     auth_tlr; */  /* if auth_len != 0 */
    } hdr;
} rpc_cn_fault_hdr_t, *rpc_cn_fault_hdr_p_t;

/*
 * Size of the fault header, excluding stub data.
 */

#define RPC_CN_PKT_SIZEOF_FAULT_HDR     (RPC_CN_PKT_SIZEOF_COMMON_HDR + 4 + 2 + 1 + 1 + 4 + 4)


/*
 *****************************************************************************
 *
 * The following packet definitions are based on the previous types.
 *
 *****************************************************************************
 */

/*
 * This is the rpc_shutdown packet header template.
 */

typedef rpc_cn_common_hdr_t     rpc_cn_shutdown_hdr_t, *rpc_cn_shutdown_hdr_p_t;
#define RPC_CN_PKT_SIZEOF_SHUTDOWN_HDR  RPC_CN_PKT_SIZEOF_COMMON_HDR

/*
 * This is the rpc_alter_context packet header template.
 */

typedef rpc_cn_bind_hdr_t rpc_cn_alter_cont_hdr_t, *rpc_cn_alter_cont_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_ALT_CTX_HDR   RPC_CN_PKT_SIZEOF_BIND_HDR

/*
 * This is the rpc_alter_context_response packet header template.
 */

typedef rpc_cn_bind_ack_hdr_t rpc_cn_alter_cont_resp_hdr_t, *rpc_cn_alter_cont_resp_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_ALT_CTX_R_HDR RPC_CN_PKT_SIZEOF_BIND_ACK_HDR

/*
 * This is the rpc_remote_alert packet header template.
 */

typedef rpc_cn_shutdown_hdr_t rpc_cn_remote_alert_hdr_t,  *rpc_cn_remote_alert_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_ALERT_HDR     RPC_CN_PKT_SIZEOF_SHUTDOWN_HDR

/*
 * This is the rpc_orphaned packet header template.
 */

typedef rpc_cn_shutdown_hdr_t rpc_cn_orphaned_hdr_t, *rpc_cn_orphaned_hdr_p_t;

#define RPC_CN_PKT_SIZEOF_ORPHANED_HDR  RPC_CN_PKT_SIZEOF_SHUTDOWN_HDR


/*
 *****************************************************************************
 *
 * One last structure which is the union of all the packets that we know.
 *
 *****************************************************************************
 */

typedef union
{

    /*
     * These are the fundamental packet types.
     */
    rpc_cn_bind_hdr_t           bind;
    rpc_cn_bind_ack_hdr_t       bind_ack;
    rpc_cn_bind_nack_hdr_t      bind_nack;
    rpc_cn_auth3_hdr_t          auth3;
    rpc_cn_request_hdr_t        request;
    rpc_cn_response_hdr_t       response;
    rpc_cn_fault_hdr_t          fault;

    /*
     * These are based on the fundamental packet types.
     */
    rpc_cn_shutdown_hdr_t       shutdown;
    rpc_cn_alter_cont_hdr_t     alter_context;
    rpc_cn_alter_cont_resp_hdr_t alter_context_response;
    rpc_cn_orphaned_hdr_t       orphaned;
} rpc_cn_packet_t, *rpc_cn_packet_p_t;

#define RPC_CN_HDR_BIND(pkt_p)  ((pkt_p)->bind)
#define RPC_CN_HDR_BACK(pkt_p)  ((pkt_p)->bind_ack)
#define RPC_CN_HDR_BNACK(pkt_p) ((pkt_p)->bind_nack)
#define RPC_CN_HDR_AUTH3(pkt_p) ((pkt_p)->auth3)
#define RPC_CN_HDR_RQST(pkt_p)  ((pkt_p)->request)
#define RPC_CN_HDR_RESP(pkt_p)  ((pkt_p)->response)
#define RPC_CN_HDR_FAULT(pkt_p) ((pkt_p)->fault)

/* common part of packet header */

#define RPC_CN_PKT_VERS(pkt_p)            (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.rpc_vers)
#define RPC_CN_PKT_VERS_MINOR(pkt_p)      (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.rpc_vers_minor)
#define RPC_CN_PKT_PTYPE(pkt_p)           (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.ptype)
#define RPC_CN_PKT_FLAGS(pkt_p)           (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.flags)
#define RPC_CN_PKT_DREP(pkt_p)            (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.drep)
#define RPC_CN_PKT_FRAG_LEN(pkt_p)        (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.frag_len)
#define RPC_CN_PKT_AUTH_LEN(pkt_p)        (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.auth_len)
#define RPC_CN_PKT_CALL_ID(pkt_p)         (RPC_CN_HDR_BIND(pkt_p).hdr.common_hdr.call_id)

/* bind and bind_ack */

#define RPC_CN_PKT_MAX_XMIT_FRAG(pkt_p)   (RPC_CN_HDR_BIND(pkt_p).hdr.max_xmit_frag)
#define RPC_CN_PKT_MAX_RECV_FRAG(pkt_p)   (RPC_CN_HDR_BIND(pkt_p).hdr.max_recv_frag)
#define RPC_CN_PKT_ASSOC_GROUP_ID(pkt_p)  (RPC_CN_HDR_BIND(pkt_p).hdr.assoc_group_id)

/* bind_ack */

/* bind_nack */

#define RPC_CN_PKT_PROV_REJ_REASON(pkt_p) (RPC_CN_HDR_BNACK(pkt_p).hdr.provider_reject_reason)

#define RPC_CN_PKT_VERSIONS(pkt_p)        (RPC_CN_HDR_BNACK(pkt_p).hdr.versions)

/* auth3 */

#define RPC_CN_PKT_AUTH3_MAX_XMIT_FRAG(pkt_p)   (RPC_CN_HDR_AUTH3(pkt_p).hdr.max_xmit_frag)
#define RPC_CN_PKT_AUTH3_MAX_RECV_FRAG(pkt_p)   (RPC_CN_HDR_AUTH3(pkt_p).hdr.max_recv_frag)

/* request */

#define RPC_CN_PKT_OPNUM(pkt_p)           (RPC_CN_HDR_RQST(pkt_p).hdr.opnum)

#define RPC_CN_PKT_OBJECT(pkt_p)          (RPC_CN_HDR_RQST(pkt_p).hdr.object.object)

/*
 * Start of the stub data field in a request packet.  The two
 * variations are for those with and without the object UUID field.
 */

#define RPC_CN_PKT_RQST_STUB_DATA_NO_OBJ(pkt_p)\
    (RPC_CN_HDR_RQST(pkt_p).hdr.object.stub_data_no_object)

#define RPC_CN_PKT_RQST_STUB_DATA_W_OBJ(pkt_p)\
    (RPC_CN_HDR_RQST(pkt_p).hdr.stub_data_w_object)

/* request, response & fault */

#define RPC_CN_PKT_ALLOC_HINT(pkt_p)      (RPC_CN_HDR_RQST(pkt_p).hdr.alloc_hint)

#define RPC_CN_PKT_PRES_CONT_ID(pkt_p)    (RPC_CN_HDR_RQST(pkt_p).hdr.pres_cont_id)

/* response & fault */

#define RPC_CN_PKT_ALERT_COUNT(pkt_p)     (RPC_CN_HDR_RESP(pkt_p).hdr.alert_count)

#define RPC_CN_PKT_RESP_RSVD(pkt_p)       (RPC_CN_HDR_RESP(pkt_p).hdr.reserved)

#define RPC_CN_PKT_RESP_STUB_DATA(pkt_p)\
    (RPC_CN_HDR_RESP(pkt_p).hdr.stub_data)

/* fault */

#define RPC_CN_PKT_STATUS(pkt_p)          (RPC_CN_HDR_FAULT(pkt_p).hdr.status)

#define RPC_CN_PKT_RESP_RSVD2(pkt_p)      (RPC_CN_HDR_FAULT(pkt_p).hdr.reserved2)

#define RPC_CN_PKT_FAULT_STUB_DATA(pkt_p)\
    (RPC_CN_HDR_FAULT(pkt_p).hdr.stub_data)

/*
 * R P C _ G _ C N _ C O M M O N _ H D R 
 */
EXTERNAL rpc_cn_common_hdr_t rpc_g_cn_common_hdr;


/*
 * R P C _ C N _ U N P A C K _ H D R 
 */

PRIVATE void rpc__cn_unpack_hdr (rpc_cn_packet_p_t);

/*
 * R P C _ C N _ P K T _ F O R M A T _ C O M M O N
 */

PRIVATE void rpc__cn_pkt_format_common (
        rpc_cn_packet_p_t       /* pkt_p */,
        unsigned32              /* ptype */,
        unsigned32              /* flags */,
        unsigned32              /* frag_len */,
        unsigned32              /* auth_len */,
        unsigned32              /* call_id */,
        unsigned8               /* minor_version */
    );

/*
 * R P C _ C N _ S T A T S _ P R I N T
 */

PRIVATE void rpc__cn_stats_print (void );

/*
 * R P C _ C N _ P K T _ N A M E
 */
PRIVATE char *rpc__cn_pkt_name ( unsigned32);


/*
 * R P C _ C N _ P K T _ C R C _ C O M P U T E
 */
PRIVATE unsigned32 rpc__cn_pkt_crc_compute (
        unsigned8       * /* block */,
        unsigned32      /* block_len */
    );

#endif /* _CNPKT_H */
