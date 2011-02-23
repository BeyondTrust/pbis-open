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
#ifndef _DG_H
#define _DG_H
/*
**
**  NAME:
**
**      dg.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Declarations of types/constants related to the NCA RPC datagram protocol.
**
**
*/

/* ========================================================================= */

#include <commonp.h>
#include <com.h>
#include <ndrp.h>
#include <ndrglob.h>

/* ========================================================================= */

/*
 * Some terminology conventions in procedure naming.
 *
 * Below, by "collection" ("C") we mean a table of some sort; collections
 * are composed of "elements".  The Client Connection Table (CCT) is
 * an example of a collection; a client connection table entry (CCTE)
 * is an example of an element.  By "object" ("O") we mean some data
 * structure.  A call handle is an example of an object.
 *
 * C_lookup
 *      means look something up in the "C" collection.  If a matching
 *      element is found, return the element.  Otherwise return NULL.
 * 
 * C_insert 
 *      means insert an element into the "C" collection.
 *
 * C_remove
 *      means remove an element from the "C" collection.
 * 
 * C_get
 *      means look something up in the "C" collection.  If a matching
 *      element is found, return the element.  Otherwise, create an
 *      appropriate element, insert it in the collection and return the
 *      element.
 *
 * O_release
 *      means release a reference to an "O" object.  (If "O" refers to
 *      a collection, the object is an element in the collection previously
 *      obtained via either "C_lookup" or "C_get".)  See "Reference Counts"
 *      comment below.
 * 
 * O_reference  
 *      means establish another reference to an "O" object.  (If "O"
 *      refers to a collection, the object is an element in the collection
 *      previously obtained via either "C_lookup" or "C_get".)  See
 *      "Reference Counts" comment below.
 * 
 * O_alloc
 *      means allocate and return (a reference to) an "O" object.
 *      
 * O_free
 *      means deallocate an "O" object.
 *      
 * O_init
 *      means initialize an "O" object.
 *      
 * O_reinit
 *      means re-initialize an "O" object.
 *
 * In general "O_foo" operates on an "O".  This is the "object-oriented"
 * approach.  However, we don't belabor it -- some procedures are just
 * not best considered as operations on objects; i.e., they're procedures,
 * not operations (messages, in the Smalltalk sense).  For just procedures,
 * we use English-oriented names (whatever that means).
 */

/* ========================================================================= */

/* 
 * Reference Counts
 * ----------------
 * 
 * Various data structures have a reference count field named ".refcnt".
 * Reference counting augments (does not replace) mutex locking in situations
 * where (a) a lock would have to be held for "too long", (b) the data
 * structure is heap allocated, and (c) the data structure should not be
 * freed until all the logical users of the structure are done with it.
 * The paradigmatic code sequence that requires reference counts is:
 * 
 *      (1) Lock data structure
 *      (2) Look at interesting parts of structure
 *      (3) Do something for a long time
 *      (4) Look at interesting parts of structure
 *      (5) Unlock structure
 * 
 * We avoid the "holding a lock for too long" problem in the above code
 * by doing:
 * 
 *      (1) Lock data structure
 *      (2) Look at interesting parts of structure
 *      (3) "reference" 
 *      (4) Unlock structure
 *      (5) Do something for a long time
 *      (6) Lock data structure
 *      (7) Look at interesting parts of structure
 *      (8) "release"
 * 
 * "Reference" means increment the structure's reference count; "release"
 * means decrement the structure's reference count and if the count becomes
 * zero then free the structure, otherwise just unlock the structure.  In
 * this model no one directly calls a "free" function; they call a "release"
 * function instead (which in turn does a "free" if the reference count
 * drops to zero).
 * 
 * Note that the code above is really an execution flow and does not
 * necessarily reflect code structure.  I.e., steps (1)-(4) might be in
 * one place, step (5) in another, and steps (6)-(8) in yet another.  The
 * basic idea is this:  If you establish a reference to a structure (e.g.,
 * by embedding a pointer to the structure in some other structure, or if
 * you "pass" a pointer to another thread somehow) and you unlock the
 * structure and you want the structure to be there later, you must increment
 * the reference count.
 * 
 * Routines that look things up and return references to entries should
 * return with the entry locked and the reference count already incremented.
 * 
 * To establish or release a reference or to free a structure, you must
 * have the structure locked.
 *
 * Since releasing a reference will unlock or free a structure, you must
 * not use the reference after a release.
 */

/* ========================================================================= */

/* !!! */

#define UUID_NIL uuid_g_nil_uuid

/* ========================================================================= */

/*
 * In some cases it is inconvenient/wasteful to be dynamically allocating
 * storage to hold network addresses.  For example, each rqe in the packet
 * buffer cache contains a network address field;  adhering to the
 * prescribed use of rpc_addr_p_t's could result in having to reallocate
 * this buffer each time a packet is received on a different NAF.  To
 * avoid this situation, we define an RPC address structure that should
 * be large enough to hold the largest network address we expect to see.
 * The size of the actual address part of this type is determined by the
 * value of the constant MAX_NETADDR_LENGTH, which can be defined in
 * the system specific include file for any system which does not want
 * to use the default value (defined below).
 */    

#ifndef MAX_NETADDR_LENGTH
# define MAX_NETADDR_LENGTH 14
#endif

typedef struct
{
    rpc_protseq_id_t        rpc_protseq_id;
    unsigned32          len;
    struct 
    {
        unsigned16          family;
        unsigned8           data[MAX_NETADDR_LENGTH]; 
    } sa;
} rpc_addr_il_t, *rpc_addr_il_p_t;

/* ========================================================================= */

/*
 * RPC protocol ID for DG protocol.  For tower rep.
 */

#define RPC_C_DG_PROTO_ID 0x0a

/* ========================================================================= */

/*
 * Packet format definitions
 */

/*
 * The current version of the NCA/RPC protocol.  See RPC_DG_HDR_{INQ,SET}_VERS
 * macros below.
 */

#define RPC_C_DG_PROTO_VERS 4


/*
 * Max packet size
 *
 * This is the size of the largest RPC packet (including RPC packet header
 * and body) we can cope with.  This value must be 0 MOD 8 (see discussion
 * in packet header comments below).  This is an IMPLEMENTATION artifact
 * and does not represent the largest packet the protocol supports.
 *
 * We assume that the packet buffer memory is cheap in the user
 * space and use the packet buffer size which can hold a single IP
 * fragment on the FDDI network.
 */

#define RPC_C_DG_MAX_PKT_SIZE  (540 * 8) /* = 4320 */

/*
 * The different kinds of RPC packets, annotated as to which direction
 * they're sent.  See RPC_DG_HDR_{INQ,SET}_PTYPE macros below.
 */

typedef unsigned32 rpc_dg_ptype_t;

#define RPC_C_DG_PT_REQUEST           0 /* client -> server */
#define RPC_C_DG_PT_PING              1 /* client -> server */
#define RPC_C_DG_PT_RESPONSE          2 /* server -> client */
#define RPC_C_DG_PT_FAULT             3 /* server -> client */
#define RPC_C_DG_PT_WORKING           4 /* server -> client */
#define RPC_C_DG_PT_NOCALL            5 /* server -> client */
#define RPC_C_DG_PT_REJECT            6 /* server -> client */
#define RPC_C_DG_PT_ACK               7 /* client -> server */
#define RPC_C_DG_PT_QUIT              8 /* client -> server */
#define RPC_C_DG_PT_FACK              9 /* both directions  */
#define RPC_C_DG_PT_QUACK            10 /* server -> client */

#define RPC_C_DG_PT_MAX_TYPE RPC_C_DG_PT_QUACK

/*
 * Macros to (hopefully efficiently) test packet types.
 */

#define RPC_DG_PT_IS(ptype, mask) (((1 << (ptype)) & (mask)) != 0)

#define RPC_DG_PT_IS_DATA(ptype) \
    RPC_DG_PT_IS(ptype, \
        (1 << RPC_C_DG_PT_REQUEST)  | \
        (1 << RPC_C_DG_PT_RESPONSE) | \
        (1 << RPC_C_DG_PT_FAULT)      \
    )

#define RPC_DG_PT_IS_CTOS(ptype) \
    RPC_DG_PT_IS(ptype, \
        (1 << RPC_C_DG_PT_REQUEST)  | \
        (1 << RPC_C_DG_PT_PING)     | \
        (1 << RPC_C_DG_PT_ACK)      | \
        (1 << RPC_C_DG_PT_QUIT)       \
    )

#define RPC_DG_PT_IS_STOC(ptype) \
    RPC_DG_PT_IS(ptype, \
        (1 << RPC_C_DG_PT_RESPONSE) | \
        (1 << RPC_C_DG_PT_FAULT)    | \
        (1 << RPC_C_DG_PT_WORKING)  | \
        (1 << RPC_C_DG_PT_NOCALL)   | \
        (1 << RPC_C_DG_PT_REJECT)   | \
        (1 << RPC_C_DG_PT_QUACK)      \
    )

/*
 * Packet header structure.
 *
 * Alignment
 * ---------
 * All the scalar fields are "naturally aligned" -- i.e. aligned 0 MOD
 * min(8, sizeof(field)).  This is done for maximum portability and
 * efficiency of field reference.  Note also that the header is a integral
 * multiple of 8 bytes in length.  This ensures that (assuming the pkt
 * is 8-byte aligned in memory), the start of the data area is 8-byte
 * aligned.  (8 bytes is the size of the largest scalar we support.)
 *
 * Wrapping
 * --------
 * Sequence and fragment numbers can wrap.  Non-equality comparisons
 * between such objects must use the comparison macros below.
 * 
 * Fragment numbers
 * ----------------
 * In requests, "fragnum" is the number of the frag sent.  In facks,
 * "fragnum" is the number of the highest in-order frag received; if
 * frag 0 hasn't been received, the fack contains a "fragnum" of 0xffff.
 * 
 *
 * Packet lengths
 * --------------
 * Packet bodies must have lengths that are 0 MOD 8, except for the last
 * fragment in a request or response.  (Since the packet header is also
 * of a length that is 0 MOD 8, this means that the entire packet will
 * be of a length that is 0 MOD 8.)  Rationale: The stub/runtime interface
 * demands that we return data buffers whose length is 0 MOD 8.  (This
 * means that no NDR scalar will ever be split across buffers, a convenient
 * property for the stubs.)  We make it easy to meet this demand by making
 * packets of length 0 MOD 8, meaning we can simply return (pointer to)
 * packets as received data buffers to the stubs.
 * 
 * Storage layout
 * --------------
 * Not all C compilers (esp. those for machines whose smallest addressible
 * unit is not 8 bits) pack the "rpc_dg_pkt_hdr_t" structure "correctly"
 * (i.e. into a storage layout that can be overlayed on a vector of bytes
 * that make up a packet that's just come off the wire).  Thus, on some
 * machines "rpc_dg_pkt_hdr_t" can not simply be used on incoming packets
 * (or used to set up outgoing packets).  We call machines that have
 * this problem "mispacked header machines".
 * 
 * To fix this problem, on mispacked header machine, when a packet arrives
 * we expand the header bits so they correspond to the actual storage
 * layout of "rpc_dg_pkt_hdr_t".  Analogously, before sending a packet,
 * we contract the header bits.
 */

typedef struct {
    u_char _rpc_vers;       /* 00:01 RPC version (see RPC_DG_HDR_*_VERS below) */
    u_char _ptype;          /* 01:01 packet type (see RPC_HDR_HDR_*_PTYPE below) */
    u_char flags;           /* 02:01 flags (see rpc_c_dg_pf_... below) */
    u_char flags2;          /* 03:01 more flag (see rpc_c_dg_pf2__... below) */
    u_char drep[3];         /* 04:03 data type format of sender (see below) */
    u_char serial_hi;       /* 07:01 high byte of fragment serial number */
    dce_uuid_t object;          /* 08:16 object UID */
    dce_uuid_t if_id;           /* 24:16 interface UID */
    dce_uuid_t actuid;          /* 40:16 activity UID of caller */
    unsigned32 server_boot; /* 56:04 time server booted */
    unsigned32 if_vers;     /* 60:04 version of interface */
    unsigned32 seq;         /* 64:04 sequence # -- monotonically increasing */
    unsigned16 opnum;       /* 68:02 operation # within the trait */
    unsigned16 ihint;       /* 70:02 interface hint (which interface w/in server) */
    unsigned16 ahint;       /* 72:02 activity hint */
    unsigned16 len;         /* 74:02 length of body */
    unsigned16 fragnum;     /* 76:02 fragment # */
    u_char auth_proto;      /* 78:01 authentication protocol */
    u_char serial_lo;       /* 79:01 low byte of fragment serial number */
} rpc_dg_pkt_hdr_t, *rpc_dg_pkt_hdr_p_t;

/*
 * Macros for accessing the "rpc_vers" field.  These macros operate on
 * only the low 4 bits of the field, yielding possible versions in the
 * range (0..15).  While pre-NCS 2.0 implementations look at all 8 bits,
 * using these macros may allow us some flexibility in the use of these
 * bits in environments where all systems are NCS 2.0 or later.  NOTE
 * WELL, that the "set" macro sets ALL 8 bits (i.e., the high 4 bits to
 * zero), so when/if we want to use those bits, we have to visit all
 * the callers of that macro (which we'd presumably have to do in any
 * case).
 */

#define RPC_C_DG_VERS_MASK  0x0f    /* 4 bits wide */

#define RPC_DG_HDR_INQ_VERS(hdrp) \
    ((hdrp)->_rpc_vers & RPC_C_DG_VERS_MASK)
#define RPC_DG_HDR_SET_VERS(hdrp) \
    ((hdrp)->_rpc_vers = RPC_C_DG_PROTO_VERS)

/*
 * Macros for accessing the "ptype" field.  These macros operate on only
 * the low 5 bits of the field, yielding possible packet types in the
 * range (0..31).  While pre-NCS 2.0 implementations look at all 8 bits,
 * using these macros may allow us some flexibility in the use of these
 * bits in environments where all systems are NCS 2.0 or later.  NOTE
 * WELL, that the "set" macro sets ALL 8 bits (i.e., the high 3 bits to
 * zero), so when/if we want to use those bits, we have to visit all
 * the callers of that macro (which we'd presumably have to do in any
 * case).
 */

#define RPC_C_DG_PTYPE_MASK 0x1f    /* 5 bits wide */

#define RPC_DG_HDR_INQ_PTYPE(hdrp) \
    ((hdrp)->_ptype & RPC_C_DG_PTYPE_MASK)

#define RPC_DG_HDR_SET_PTYPE(hdrp, ptype) \
    ((hdrp)->_ptype = (ptype))

/*
 * Initial value for ihint and ahint fields.
 */

#define RPC_C_DG_NO_HINT ((unsigned16) 0xffff)

/*
 * Packet flags (used in "flags" field in packet header), indicating
 * in packets and in which direction(s)--i.e., client->server,
 * server->client, or both--they're used.  Currently, no flag is defined
 * to have one meaning in one direction and another meaning in the other
 * direction; i.e., if a flag is used in both directions, it means the
 * same thing in both directions.  However, to allow for the possibility
 * in the future of flags meaning one thing in one direction and another
 * thing in the other, all flags that are currently used in only one
 * direction must be set to zero when sent in the other direction.
 * 
 * Note on rpc_c_dg_pf_blast_outs:  Early versions of NCK did not blast
 * large ins/outs but were designed to work with senders that did blast
 * large ins/outs.  (I.e. early versions supported the rpc_c_dg_pf_no_fack
 * bit which is now used by new senders that want to blast multiple frags.)
 * Unfortunately, while the design was correct, the implementation wasn't.
 * Old clients have a bug in handling blasted outs.  So by default, new
 * servers won't blast large outs.  The rpc_c_dg_pf_blast_outs flag is
 * set by new clients to tell the server that the client can handle blasts.
 * 
 * Note on forwarding:  Forwarding is a purely intra-machine function;
 * i.e. The rpc_c_dg_pf_forwarded and rpc_c_dg_pf2_forwarded2 bits should
 * never be set on packets sent on the wire.  It's pretty sleazy to be
 * stealing these bits (i.e. make them unavailable for the on-the-wire
 * protocol), but that's life.  If in the future we're willing to go
 * through more work (i.e. do intra-machine forwarding through some other
 * means), we could free up those bits.  See comments below for details
 * on use of the bits.
 */

#define RPC_C_DG_PF_FORWARDED    0x01        /* (client -> server) Packet was forwarded */
#define RPC_C_DG_PF_LAST_FRAG    0x02        /* (both directions)  Packet is the last fragment */
#define RPC_C_DG_PF_FRAG         0x04        /* (both directions)  Packet is a fragment */
#define RPC_C_DG_PF_NO_FACK      0x08        /* (both directions)  Don't send an FACK for this FRAG */
#define RPC_C_DG_PF_MAYBE        0x10        /* (client -> server) "maybe" request */
#define RPC_C_DG_PF_IDEMPOTENT   0x20        /* (client -> server) "idempotent" request */
#define RPC_C_DG_PF_BROADCAST    0x40        /* (client -> server) "broadcast" request */
#define RPC_C_DG_PF_BLAST_OUTS   0x80        /* (client -> server) out's can be blasted */

/*
 * Packet flags (used in "flags2" field in packet header).
 */

#define RPC_C_DG_PF2_FORWARDED_2 0x01        /* (client -> server) Packet is being forwarded in two pieces */
#define RPC_C_DG_PF2_CANCEL_PENDING 0x02     /* (server -> client) Cancel was pending at call end */
#define RPC_C_DG_PF2_RESERVED04  0x04
#define RPC_C_DG_PF2_RESERVED08  0x08
#define RPC_C_DG_PF2_RESERVED10  0x10
#define RPC_C_DG_PF2_RESERVED20  0x20
#define RPC_C_DG_PF2_RESERVED40  0x40
#define RPC_C_DG_PF2_RESERVED80  0x80

/*
 * Macros for munging with packet flags
 */

#define RPC_DG_HDR_FLAG_IS_SET(hdrp, flag)  (((hdrp)->flags  & (flag)) != 0)
#define RPC_DG_HDR_FLAG2_IS_SET(hdrp, flag) (((hdrp)->flags2 & (flag)) != 0)

#define RPC_DG_FLAG_IS_SET(flags, flag)   ((flags  & (flag)) != 0)
#define RPC_DG_FLAG2_IS_SET(flags2, flag) ((flags2 & (flag)) != 0)

/*
 * Macros to help do A < B and A <= B compares on unsigned numbers in
 * general and specifically packet fragment and sequence numbers.  These
 * macros are useful when the numbers being compared are allowed to wrap
 * (e.g. we really want 0xfffffffe to be less than 0x00000002).
 * 
 * To perform such a comparison, we count the fact that the number space
 * for a given data type / object is large enough that we can assert:
 * if the unsigned difference A - B is "negative", A is < B.  We define
 * "negative" as being a unsigned difference that when converted to
 * a signed type of the same precision yields a negative value.
 */

#define RPC_DG_UNSIGNED_IS_LT(a, b, signed_cast)    (((signed_cast) ((a) - (b))) < 0)
#define RPC_DG_UNSIGNED_IS_LTE(a, b, signed_cast)   (((signed_cast) ((a) - (b))) <= 0)

#define RPC_DG_FRAGNUM_IS_LT(a, b)  RPC_DG_UNSIGNED_IS_LT(a, b, signed16)
#define RPC_DG_FRAGNUM_IS_LTE(a, b) RPC_DG_UNSIGNED_IS_LTE(a, b, signed16)

#define RPC_DG_SERIAL_IS_LT(a, b)  RPC_DG_UNSIGNED_IS_LT(a, b, signed16)
#define RPC_DG_SERIAL_IS_LTE(a, b) RPC_DG_UNSIGNED_IS_LTE(a, b, signed16)

#define RPC_DG_SEQ_IS_LT(a, b)      RPC_DG_UNSIGNED_IS_LT(a, b, signed32)
#define RPC_DG_SEQ_IS_LTE(a, b)     RPC_DG_UNSIGNED_IS_LTE(a, b, signed32)

/*
 * Macros to manipulate the packet header NDR drep.
 *
 * We "stole" the last byte of the 4 byte drep (which is currently
 * "reserved" and not likely to ever actually be used) to hold the
 * authentication type.  (See "ndrp.h" for details on NDR dreps.)  As
 * a result, we can't use all the macros in "ndrp.h".  We define our
 * own here.  (We #undef NDR_COPY_DREP to make sure we don't use it by
 * mistake.)
 */

#ifdef NDR_COPY_DREP
#  undef NDR_COPY_DREP
#endif

#define RPC_DG_HDR_SET_DREP(hdrp) { \
    (hdrp)->drep[0] = ndr_g_local_drep_packed[0]; \
    (hdrp)->drep[1] = ndr_g_local_drep_packed[1]; \
    (hdrp)->drep[2] = ndr_g_local_drep_packed[2]; \
}

#define RPC_DG_HDR_INQ_DREP(dst_drep, src_hdrp) { \
    (dst_drep)->int_rep   = NDR_DREP_INT_REP((src_hdrp)->drep); \
    (dst_drep)->char_rep  = NDR_DREP_CHAR_REP((src_hdrp)->drep); \
    (dst_drep)->float_rep = NDR_DREP_FLOAT_REP((src_hdrp)->drep); \
    (dst_drep)->reserved  = 0; \
}

/*
 * Raw packet header
 *
 * "rpc_c_dg_raw_pkt_hdr_size" is the actual size of a packet header
 * (in bytes) as it appears on the wire.  The size of "rpc_dg_pkt_hdr_t"
 * can only be the same or larger.  "rpc_dg_raw_pkt_hdr" is the struct
 * used for declaring a real wire-format packet header.
 *
 *  #ifndef RPC_MISPACKED_HDR
 *      assert(RPC_C_DG_RAW_PKT_HDR_SIZE == sizeof(rpc_dg_pkt_hdr_t))
 *  #endif
 *
 * The "rpc_c_dg_rpho_..." constants ("rpho" = "raw packet header offset")
 * can be used to locate the logical header fields in a raw packet header.
 */

#define RPC_C_DG_RAW_PKT_HDR_SIZE 80UL

typedef struct {
    char hdr[RPC_C_DG_RAW_PKT_HDR_SIZE];
} rpc_dg_raw_pkt_hdr_t, *rpc_dg_raw_pkt_hdr_p_t;

#define RPC_C_DG_RPHO_RPC_VERS         0
#define RPC_C_DG_RPHO_PTYPE            1
#define RPC_C_DG_RPHO_FLAGS            2
#define RPC_C_DG_RPHO_FLAGS2           3
#define RPC_C_DG_RPHO_DREP             4
#define RPC_C_DG_RPHO_SERIAL_HI        7
#define RPC_C_DG_RPHO_OBJECT           8
#define RPC_C_DG_RPHO_IF_ID           24
#define RPC_C_DG_RPHO_ACTUID          40
#define RPC_C_DG_RPHO_SERVER_BOOT     56
#define RPC_C_DG_RPHO_IF_VERS         60
#define RPC_C_DG_RPHO_SEQ             64
#define RPC_C_DG_RPHO_OPNUM           68
#define RPC_C_DG_RPHO_IHINT           70
#define RPC_C_DG_RPHO_AHINT           72
#define RPC_C_DG_RPHO_LEN             74
#define RPC_C_DG_RPHO_FRAGNUM         76
#define RPC_C_DG_RPHO_AUTH_PROTO      78
#define RPC_C_DG_RPHO_SERIAL_LO       79

/*
 * Packet body structure
 *
 * The "body" part of a pkt is used only for certain pkt types (e.g.
 * request, response).
 */

typedef struct {
    char args[RPC_C_DG_MAX_PKT_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE];
} rpc_dg_pkt_body_t, *rpc_dg_pkt_body_p_t;

#define RPC_C_DG_MAX_PKT_BODY_SIZE \
    (RPC_C_DG_MAX_PKT_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE)

#if (RPC_C_DG_MAX_PKT_BODY_SIZE & 0x7) != 0
#error RPC_C_DG_MAX_PKT_BODY_SIZE must be 0 MOD 8!
#endif

/*
 * Packet structure
 */

typedef struct
{
    rpc_dg_pkt_hdr_t hdr;
    rpc_dg_pkt_body_t body;
} rpc_dg_pkt_t, *rpc_dg_pkt_p_t;

/*
 * Error packet body structure
 *
 * Used for packets with a body that only consist of an error status 
 * (i.e. reject and fault packets).
 */

typedef struct
{
    unsigned32 st;
} rpc_dg_epkt_body_t, *rpc_dg_epkt_body_p_t;

#define RPC_C_DG_RAW_EPKT_BODY_SIZE 4   /* sizeof unsigned32     on the wire */

typedef struct
{
    u_char body[RPC_C_DG_RAW_EPKT_BODY_SIZE];
} rpc_dg_raw_epkt_body_t, *rpc_dg_raw_epkt_body_p_t;


/*
 * Quit packet body structure.  This is currently used for
 * NCS 2.0 "cancel-requests".
 */

typedef struct 
{
    unsigned32  vers;       /* quit body format version */
    unsigned32  cancel_id;  /* id of a cancel-request event */
    /* end of version 0 format */
} rpc_dg_quitpkt_body_t, *rpc_dg_quitpkt_body_p_t;

#define RPC_C_DG_QUITPKT_BODY_VERS   0

#define RPC_C_DG_RAW_QUITPKT_BODY_SIZE    (4+4)   /* on-the-wire size */

typedef struct
{
    u_char body[RPC_C_DG_RAW_QUITPKT_BODY_SIZE];
} rpc_dg_raw_quitpkt_body_t, *rpc_dg_raw_quitpkt_body_p_t;


/*
 * Quack packet body structure.  This is currently used for
 * NCS 2.0 "cancel-request" acknowlegements.
 */

typedef struct 
{
    unsigned32  vers;       /* cancel-request body format version */
    unsigned32  cancel_id;  /* id of a cancel-request event being ack'd */
    boolean     server_is_accepting;    /* NIDL boolean */
    /* end of version 0 format */
} rpc_dg_quackpkt_body_t, *rpc_dg_quackpkt_body_p_t;

#define RPC_C_DG_QUACKPKT_BODY_VERS   0

#define RPC_C_DG_RAW_QUACKPKT_BODY_SIZE    (4+4+1)  /* on-the-wire size */

typedef struct
{
    u_char body[RPC_C_DG_RAW_QUACKPKT_BODY_SIZE];
} rpc_dg_raw_quackpkt_body_t, *rpc_dg_raw_quackpkt_body_p_t;


/*
 * Fack packet body structure. 
 *
 * Pre-v2 NCK facks always had zero length bodies.  V2 fack bodies
 * contain useful information.
 *
 * The following two values related to packet size can appear in a fack
 * body:
 *
 * max_tsdu:      The size of the largest transport protocol data unit
 *                (TPDU) that can be passed through the local transport
 *                service interface (i.e., transport API).  The fack
 *                sender can never process datagrams larger than this.
 *
 * max_frag_size: The size of the largest fragment that can be
 *                sent/received by the fack sender.
 *                (AES spells it as max_path_tpdu.)
 *
 * The sender of a fack puts its local idea of these two values into
 * the fack body, to help the other side determine what fragment
 * size to use. The fack sender is saying "Don't ever send me
 * fragments larger than 'max_tsdu'" and "I think you probably don't
 * want to send me fragments larger than 'max_frag_size' because I
 * can't process it".
 *
 * The receiver of a fack compares the two values with the locally
 * determined values (in the receiver's call xmitq) and, if
 * appropriate, the xmitq's fragment size, 'snd_frag_size', is
 * increased. (Currently, we play it safe by taking the minimum the
 * two values in the fack and the two locally computed values.)
 *
 * The serial_num field is taken from the packet header of the packet
 * that induced the ack.  Since each packet is given a unique serial
 * number prior to transmission, the sender can always determine exactly
 * which packet induced the ack.  Serial numbers play the role of logical
 * clocks, which allow the sender to determine which packets should have
 * been received given an ack for a packet sent at time n.  This is 
 * especially useful in the face of multiple retransmissions, when 
 * fragment numbers don't provide any ordering information.
 *
 * The selective ack fields provide information about what additional
 * packets were received in addition to the fragment explicitly mentioned
 * in this fack (N).  If included, each bit in the mask (B) corresponds
 * to whether of not fragment N+B was received.  Note that the low order
 * bit in the first mask will always be 0, since atleast one fragment
 * is missing, and N will always be the number of the fragment before
 * the first known missing fragment.
 *
 * Note that nocall packets can also have fack bodies.  (Pre-DCE 1.0.2
 * nocall packets always had zero length bodies.)  This is to remedy
 * a couple of flaws in the original protocol which precluded (a) sending
 * facks in response to single-packet requests, and (b) sending facks
 * in response to pings (in requests of any size).  Older clients will
 * ignore the fack body that appears in nocall packets.
 */

typedef struct
{
    unsigned8 vers;             /* Fack packet body version */
    u_char pad1;
    unsigned16 window_size;     /* Sender's receive window size (in kilobytes) */
    unsigned32 max_tsdu;        /* See above */
    unsigned32 max_frag_size;   /* See above */
    unsigned16 serial_num;      /* serial # of packet that induced this fack */
    unsigned16 selack_len;      /* number of elements in the selack array    */
    unsigned32 selack[1];       /* variable number of 32 bit selective ack   */
                                /* bit masks.                                */
} rpc_dg_fackpkt_body_t, *rpc_dg_fackpkt_body_p_t;

#define RPC_C_DG_FACKPKT_BODY_VERS      1
#define RPC_C_DG_RAW_FACKPKT_BODY_SIZE  20

typedef struct
{
    u_char body[RPC_C_DG_RAW_FACKPKT_BODY_SIZE];
} rpc_dg_raw_fackpkt_body_t, *rpc_dg_raw_fackpkt_body_p_t;

/*
 * Forwarding 
 *
 * So that clients don't need to know the actual port a server is using,
 * we provide for a "forwarder" process on the server's machine.  A
 * client that doesn't know the server's port sends to the forwarder's
 * well-known port instead.
 * 
 * When a packet is received by the forwarder process, it is forwarded
 * to the correct server process on the machine.  The server must know
 * the original sender.  However, what the server "sees" as a result
 * of the "receive" is the address of the forwarder, not the original
 * client.  The solution to this problem has two cases: the case where
 * the body of the packet to be forwarded is not full and the case where
 * it IS full.  In the non-full case, the forwarder inserts the original
 * client address at the front of the body of the packet to be forwarded.
 * (The original body is slid down.)  The rpc_c_dg_pf_forwarded bit is
 * set in the header.  When the server receives such a packet, it just
 * picks up the address from the body and slides the real body back up.
 * 
 * In the case of a full packet body, the packet must be forwarded in
 * two packets.  The forwarder makes body of the first packet contains
 * only the original client address and sets the rpc_c_dg_pf2_forwarded2
 * flag.  The body of the second packet is the original packet body;
 * no forwarding flags are set.
 */

/*
 * Forwarded packet structure
 *
 * This format for local use between a forwarding agent (LLBD) and a
 * real server.  This format does not appear on the wire.
 */

typedef struct
{
    unsigned32 len;             /* Length of addr */
    struct sockaddr addr;       /* Original sender of packet */
    u_char drep[4];             /* Original data rep (describes body) */
} rpc_dg_fpkt_hdr_t, *rpc_dg_fpkt_hdr_p_t;

typedef struct
{
    rpc_dg_pkt_hdr_t hdr;
    rpc_dg_fpkt_hdr_t fhdr;
    rpc_dg_pkt_body_t body;
} rpc_dg_fpkt_t, *rpc_dg_fpkt_p_t;

/*
 * Common raw packet structure (as sent / received over the wire)
 */

typedef struct {
    rpc_dg_raw_pkt_hdr_t hdr;
    rpc_dg_pkt_body_t body;
} rpc_dg_raw_pkt_t, *rpc_dg_raw_pkt_p_t;

/* =============================================================================== */

typedef enum {
    rpc_e_dg_recv_no_data,
    rpc_e_dg_recv_unspec_error,
    rpc_e_dg_recv_cksum_error,
    rpc_e_dg_recv_bad_ptype,
    rpc_e_dg_recv_bad_len,
    rpc_e_dg_recv_bad_version,
#ifdef NON_REENTRANT_SOCKETS
    rpc_e_dg_recv_reentered,
#endif
    rpc_e_dg_recv_ok 
} rpc_dg_recv_status_t;

/* =============================================================================== */

/*
 * Monitor liveness client rep structure
 *
 * This structure is used by the server runtime to monitor the liveness
 * of a client.  The fields of interest are a UUID, which identifies
 * a particular instance of a client address space, and a pointer to
 * a function to call should contact be lost with the client.
 *
 * This structure is primarily associated, and cached, with an SCTE.
 * In the normal case, in which liveness is not being monitored, no such
 * entry is created.  If the server stub desires the runtime to monitor
 * the client,  a callback is made to the client (if the SCTE does not
 * yet contain such an entry) to retrieve a UUID by which the server
 * can identify that client's address space (i.e., any thread within
 * that process).  The client rep is then added to a hash table, a
 * reference is placed in the SCTE, and a pointer is returned back to
 * the server stub.  The server stub can then begin monitoring the client
 * by registering a rundown function pointer through a call to the
 * monitor_liveness() routine.  A NULL rundown function pointer
 * indicates that the client is not currently being monitored.
 *
 * Note that several SCTEs may point to a single client_rep.  Since the
 * runtime has no way of mapping activity IDs to address spaces, a seperate
 * call back is required for each activity ID.  However the address space
 * UUID returned for actids in the same process will be the same, and
 * only one client rep will be created.  It is also presumed that the
 * stubs will not try to register more than one rundown funtion for a
 * given client address space.
 */
typedef struct rpc_dg_client_rep_t
{   
    struct rpc_dg_client_rep_t *next;
    dce_uuid_t                     cas_uuid;         /* UUID uniquely identifying client    */
    rpc_network_rundown_fn_t   rundown;          /* routine to call if client dies      */  
    rpc_clock_t                last_update;      /* last time we heard from this client */  
    unsigned32                 refcnt;           /* # of references to this element     */ 
} rpc_dg_client_rep_t, *rpc_dg_client_rep_p_t;  

/* ========================================================================= */

/*
 * Transmit packet queue element (XQE)
 *
 * We choose to decouple the packet body from the element itself to lessen
 * our dependency on the ideosyncracies of various memory allocation
 * mechanisms.
 *
 * A private xqelt (i.e. not on a xmitq) has no locking requirements.
 * A xqelt that is a member of a xmitq essentially becomes part of that
 * data structure and inherits the xmitq's locking requirements.
 */

typedef struct rpc_dg_xmitq_elt_t {
    struct rpc_dg_xmitq_elt_t *next;
    struct rpc_dg_xmitq_elt_t *next_rexmit; /* when on rexmit queue   */
    struct rpc_dg_xmitq_elt_t *more_data;   /* for multi-buffer fragments */
    unsigned32 frag_len;        /* fragment length, iff more_data != NULL */
    unsigned16 flags;                   /* pkt header flags */
    unsigned32 fragnum;                 /* our fragment number */
    unsigned16 serial_num;              /* our serial number */
    unsigned16 body_len;                /* # of bytes used in .body */
    rpc_dg_pkt_body_p_t body;           /* pkt body */
    unsigned in_cwindow: 1;             /* T => pkt is in cwindow size count */
} rpc_dg_xmitq_elt_t, *rpc_dg_xmitq_elt_p_t;

#define RPC_C_DG_INITIAL_REXMIT_TIMEOUT RPC_CLOCK_SEC(2)
#define RPC_C_DG_MAX_REXMIT_TIMEOUT     RPC_CLOCK_SEC(8)

/*
 * Transmit packet queue (XMITQ or XQ)
 *
 * This is a queue of structs describing packets (fragments) that can
 * (and may already have been) transmitted and are awaiting receipt
 * acknowledgement.  Receipt acknowledgements can come in several flavors
 * depending on whether you are a client or a server and whether there
 * are fragments.  Adaquate acknowledgements are one of the following
 * packets: fack, working, response, ack.  The key here is that if there
 * is data on the xq then it must be there because it can't yet be tossed;
 * it's awaiting an ack.
 *
 * All necessary encryption operations (on either just the header [for
 * integrity] or on the header and the body [for privacy]) have been
 * performed on the packets in this queue.  This queue is constructed
 * using the information present in a XMIT IO Vector Queue.  Creation
 * of Packet Queue Elements is controlled by resource consumption /
 * transmit window availability.  Since encryption is based on a packet
 * worth of data (including a complete pkt header), encryption must be
 * delayed until the complete packet is "assembled".
 * 
 * Periodically, some of the "packets" on the queue may need to be
 * retransmitted because they've timed out.  They may also need to be
 * retransmitted in response to a nocall or fack receipt.  After some
 * period of no apparent acknowledgement arrivals you have to just give
 * up.
 *
 * As fragment acknowledgements are received, the packet queue elements
 * may be freed.  The transmit window will be adjusted, thereby allowing
 * more data to be transmitted.
 *
 * Some background on packet sizes:  ".max_tsdu" is our local computation
 * of how large a packet we can fit through our local tranport API.
 * ".max_path_tpdu" is our local computation of the largest packet that
 * we can send to our peer without incurring fragmentation.
 * ".max_pkt_size" is the size of the packets we're actually currently
 * sending to our peer.  This value is maintained to be the min of
 * ".max_tsdu", ".max_path_tpdu", and the TPDU info of fack bodies we
 * receive from our peer.
 *
 * ".awaiting_ack" is true in case we're expecting some sort of
 * acknowledgement (e.g., response, fack, ack, working) from the receiver.
 * This field (in conjunction with ".awaiting_ack_timestamp") is used
 * to decide when to declare the receiver dead.
 *
 * The queue is empty when 'head' == NULL.
 *            
 * Several fields within the queue are used for adjusting the blast size.
 * This mechanism is not present for "fine tuning" the blast size; it
 * is for the case where we have a very reliable connection to a fast
 * reciever and facks are returning so quickly that we are not able to
 * grow the congestion grow (i.e. we're not able to start a pipeline
 * of outstanding packets).  ".max_blast_size" is the limit to the number
 * of packets that will be sent back-to-back.  Periodically, if we aren't
 * seeing lost packets and we notice that the congestion window is not
 * reaching the advertised window size, we will increase the blast size
 * by 2.  We may do this twice, reaching a maximum blast of 8.  The field
 * ".high_cwindow" is used to keep track of congestion window growth
 * between these periodic adjustments.  The time between adjustments
 * is controlled by ".xq_timer", a counter which gets decremented each
 * time we receive a fack which does not report a loss.  Initially we
 * check after 8 such facks have arrived.  However, each time the blast
 * size is adjusted, we increment ".xq_timer_throttle" which is used
 * as a multiplier for ".xq_timer".  That is, the periods between
 * considering a change to the blast size become longer and longer, to
 * prevent oscillating around the blast size limit.
 * 
 * The entire xmitq inherits the containing structure's locking
 * requirements and is protected by that structure's lock.
 */

typedef struct {
    rpc_dg_xmitq_elt_p_t head;          /* -> head pktq element */
    rpc_dg_xmitq_elt_p_t first_unsent;  /* -> 1st unsent pkt in the queue */
    rpc_dg_xmitq_elt_p_t tail;          /* -> tail pktq element */
    rpc_dg_xmitq_elt_p_t rexmitq;       /* -> pkts to be resent */
    rpc_dg_xmitq_elt_p_t part_xqe;      /* partially filled xmit queue element */
    rpc_dg_pkt_hdr_t hdr;               /* prototype pkt hdr */
    rpc_clock_t awaiting_ack_timestamp; /* when awaiting_ack was set */
    rpc_clock_t timestamp;              /* last (re)transmission time */
    rpc_clock_t rexmit_timeout;         /* time until next retransmission time */
    unsigned16 base_flags;              /* header flags constants for all pkts */
    unsigned16 base_flags2;             /* header flags constants for all pkts */
    unsigned16 next_fragnum;            /* next fragnum to use */
    unsigned16 next_serial_num;         /* unique serial # sent in each pkt   */
    unsigned16 last_fack_serial;        /* serial # that induced last recv'd fack */
    unsigned16 window_size;         /* receiver's window size (in fragments) */
    unsigned16 cwindow_size;        /* congestion window size (in fragments) */
    unsigned32 max_rcv_tsdu;            /* max receive data size supported by the LOCAL transport API (recvmsg) */
    unsigned32 max_snd_tsdu;            /* max send data size supported by the LOCAL transport API (sendmsg) */
    unsigned32 max_frag_size;           /* max frag size can be received/sent */
    unsigned32 snd_frag_size;           /* send frag size, currently used */
    unsigned8 blast_size;               /* # of pkts to send as a blast */
    unsigned8 max_blast_size;           /* limit to the above value */
    unsigned16 xq_timer;                /* used by blast size code to delay adjustments */
    unsigned16 xq_timer_throttle;       /* multiplier to timer, to increase delays */
    unsigned8 high_cwindow;             /* highest value of .cwindow_size so far */
    unsigned8 freqs_out;                /* # of outstanding fack requests */
    unsigned push: 1;                   /* T => don't keep at least one pkt on queue */
    unsigned awaiting_ack: 1;           /* T => we are waiting for a xq ack event */
    unsigned first_fack_seen: 1;        /* T => we saw the first fack */
} rpc_dg_xmitq_t, *rpc_dg_xmitq_p_t;

/*
 * The initial value of ".window_size".  The window size we will believe
 * the receiver is offering until we hear otherwise.
 */
#define RPC_C_DG_INITIAL_WINDOW_SIZE        8   

/*
 * The initial blast size, used when (re)starting a cwindow.
 */
#define RPC_C_DG_INITIAL_BLAST_SIZE         2

/*
 * The default maximum blast size.  For some conversations, the actual
 * blast size used may be greater.
 */
#define RPC_C_DG_INITIAL_MAX_BLAST_SIZE     4   

/*
 * The absolute maximum blast size allowable under any circumstances.
 * Any increase in this value will require revisiting the code which determines 
 * when fack requests are sent out (rpc__dg_call_xmit).
 */
#define RPC_C_DG_MAX_BLAST_SIZE             8
                                            
/*
 * The initial value of the xq_timer field.  This is the number of "good" facks
 * we must see before considering an adjustment to the blast size.
 */
#define RPC_C_DG_INITIAL_XQ_TIMER           8    
            
/*
 * The initial value of ".max_pkt_size".  The size of the largest datagrams
 * we will send unless the receiver tells us we can send larger ones.  We
 * start small to deal with pre-v2 receivers, which deal exclusively in
 * 1024 byte packets.
 */  
#define RPC_C_DG_INITIAL_MAX_PKT_SIZE       1024

/* 
 * Since the sockets used by the DG protocol are non-blocking, it is always
 * possible to receive an EWOULDBLOCK error status when trying to transmit
 * data.  Since this condition should be short-lived, what we'd like to do
 * is put the thread to sleep until the transmission can be done successfully.
 * This is fine when the send is being done from a client/call thread, since
 * the thread has nothing better to do at the time.  It *is* a problem if the 
 * send is being done from the listener or timer threads;  in these cases,
 * there is other work that the thread could be doing for other threads.
 *
 * Unfortunately, we have not been totally successful in eliminating
 * all need for data transmissions from the listener/timer threads.  The
 * one exception in the case where the last packet(s) of a call's OUTS
 * are dropped.  Because of delayed acknowledgements, it is not desirable
 * to detain the server call thread while waiting for an ack from the
 * client.  In the worst case, the client could exit without returning
 * an ACK, and the server call thread would be hung up for 30 seconds
 * before it timed out.
 * 
 * To avoid tieing up the thread, we will allow transmissions to be made
 * from the listener/timer thread in cases where the call has transitioned
 * to the final state.  However, if an EWOULDBLOCK error occurs, it will be
 * ignored, and the packet will have to wait for the next timeout to be resent.
 * This solution was deemed acceptable based on the following observations:  
 *
 *      1) All packets have been successfully sent atleast once (w/o EWOULDBLOCK)
 *      2) On a timeout, there could be at most 4 packets left to send.  The
 *         transport algorithm would restart the congestion window by sending 
 *         out two packets, both requesting facks.  Hopefully, at least one will
 *         get through.
 */
#define RPC_DG_START_XMIT(call) \
        if ((call)->state == rpc_e_dg_cs_final) \
            rpc__dg_call_xmit(call, false); \
        else \
            rpc__dg_call_signal(call);
                  
/*
 * And here's a macro to help decide if you should call the macro above.
 */
#define RPC_DG_CALL_READY_TO_SEND(call) ((call)->xq.blast_size > 0)

/* ========================================================================= */

/*
 * Receive packet queue element (RQE).
 *
 * An adorned packet.  The adornment consists of (1) a linked list pointer,
 * (2) the packet's source network address, and (3) a pointer to a "usable
 * header".  By "usable" we mean that the data rep is correct and that
 * the layout matches what the local compiler produces for the
 * "rpc_dg_pkt_hdr_t" struct.
 *
 * We choose to decouple the packet body from the element itself to lessen
 * our dependency on the idiosyncracies of various memory allocation
 * mechanisms.  ".pkt_real" points to the buffer that's been allocated
 * to hold the packet that arrives on the wire.  ".pkt" is an copy of
 * ".pkt_real" that's been aligned to a 0 mod 8 boundary.  (The stubs
 * expect data returned via "rpc_call_receive" and "rpc_call_transceive"
 * to be so aligned.)  All processing of the packet other than allocation
 * and freeing uses ".pkt", not ".pkt_real".
 *
 * ".hdrp" can point to either ".hdr" or ".pkt->hdr", depending on whether
 * the original header is usable.  This allows us to save data copies
 * in case the original header is usable.  
 * 
 * A private rqelt (i.e. not on a recvq) has no locking requirements.
 * A rqelt that is a member of a recvq essentially becomes part of that
 * data structure and inherits the recvq's locking requirements.
 */

typedef struct rpc_dg_recvq_elt_t {
    struct rpc_dg_recvq_elt_t *next;
    struct rpc_dg_recvq_elt_t *more_data;  /* for multi-buffer fragments */
    rpc_dg_pkt_hdr_p_t hdrp;            /* -> usable header info */
    rpc_dg_pkt_hdr_t hdr;               /* pkt hdr in usable form */
    rpc_socket_t sock;                  /* socket for response */
    unsigned32 frag_len;                /* length of fragment, as recv'ed
                                         * iff a head of fragment */
    unsigned16 from_len;                /* length of ".from" */
    unsigned16 pkt_len;                 /* length of .pkt, as recv'ed */
    rpc_addr_il_t from;                 /* address of sender */
    rpc_dg_raw_pkt_p_t pkt;             /* the raw received packet (offset to
                                         * 0 mod 8 boundary) */
    rpc_dg_raw_pkt_p_t pkt_real;        /* the real start of the raw packet */
} rpc_dg_recvq_elt_t, *rpc_dg_recvq_elt_p_t;
                            
/*
 * Receive packet queue (RECVQ or RQ).
 *
 * Packets are removed off the head of the list (only in-sequence fragments
 * are given to stubs).  The packets are kept in fragment number order.
 * 
 * As an optimization, a pointer tracks the "last inorder" packet on
 * the list to avoid scanning the entire list from the head to the tail
 * for the appropriate insertion point.  A queue length count is maintained
 * for resource utilization control (if the queue becomes too full,
 * received packets will just be ignored).
 * 
 * If ".last_inorder" is NULL, then there is a "gap" at the beginning
 * of the queue or there are no pkts in the queue.
 *
 * The entire recvq inherits the containing structure's locking
 * requirements and is protected by that structure's lock.
 */

typedef struct {
    rpc_dg_recvq_elt_p_t head;          /* -> head pktq element */
    rpc_dg_recvq_elt_p_t last_inorder;  /* -> last in-order frag in the queue */
    unsigned32 high_rcv_frag_size;      /* largest frag size seen so far */
    unsigned16 next_fragnum;            /* next in-order frag we want to see */
    unsigned16 high_fragnum;            /* highest numbered fragnum so far */
    unsigned16 high_serial_num;         /* highest serial number seen so far */
    unsigned16 head_fragnum;            /* fragnum for the head of queue */
    unsigned16 head_serial_num;         /* serial # for the head of queue,
                                           iff head->hdrp == NULL */
    unsigned16 window_size;             /* our receive window size (in pkts) */
    unsigned16 wake_thread_qsize;       /* # of bytes to q before waking thread */
    unsigned16 max_queue_len;           /* max. # of frags allowed on queue */
    unsigned8 queue_len;                /* total frags on queue */
    unsigned8 inorder_len;              /* total inorder frags on queue */
    unsigned recving_frags: 1;          /* T => we are receiving frags */
    unsigned all_pkts_recvd: 1;         /* T => we've recv'd all data pkts */
    unsigned is_way_validated: 1;       /* T => we've passed the WAY check */
} rpc_dg_recvq_t, *rpc_dg_recvq_p_t;
  
/*
 * Maximum number of packets we'll queue for any receiver.  This limit
 * is imposed in the interest of providing all threads with fair access
 * to RQE's.  Functionally, exceeding this limit will be handled the
 * same as if the receive packet quota had been exceeded.
 */

#define RPC_C_DG_MAX_RECVQ_LEN       96

/* ========================================================================= */

/*
 * Client connection table (CCT) and connection table elements (CCTE).
 *
 * Keeps track of all "connections" from the local client address space
 * to remote servers.  The CCT is keyed by authentication info.  The
 * CCT is a hash table with separate chaining.
 * 
 * A client wanting to make a call picks a connection to use by looking
 * up a non-inuse CCTE that has a matching authentication information
 * handle.  If it finds an entry, it uses that entry's activity ID (actid)
 * and sequence number in its call.  The client increments the sequence
 * number in the table and sets the "in use" flag for the duration of
 * the client's call.  If the client finds no matching entry, it creates
 * a new entry by generating a new UUID as the activity ID.  Maintain
 * the chains in "oldest first" order.  This will cause older available
 * entries to be reused before newer entries thereby naturally limiting
 * the ccte (actid) selection to a smaller set.
 * 
 * The CCT is garbage collectable -- slots containing entries that are
 * not "in use" can be re-assigned to new auth's.  Any deletions in the
 * CCT *must* increment the CCT's GC counter.
 * 
 * See "dgglob.h" for definition of the table itself.
 * 
 * A CCTE that is not inuse is "owned" by the CCT and inherits its locking
 * requirements.  When a CCTE is inuse (".refcnt" > 1 -- note that the
 * CCT's reference itself counts as a reference), all fields except the
 * next field are "owned" by the call that is using this connection.
 * The next field is always "owned" by the CCT.
 * 
 * The relationship between the CCTEs, activity ids, auth-infos, call
 * handles, and binding handles is important (to make things work at
 * all, as well as work efficiently).  So, here's more info on the topic:
 *
 *      Actids are the cornerstone of all client / server conversations.
 *      Servers identify clients by an actid.
 * 
 *      Actids have an associated non-decreasing RPC Call Sequence Number.
 * 
 *      A single actid may be used by a single RPC at a time.
 * 
 *      Actids have an associated authentication identity which is fixed
 *      for the lifetime of the actid.  If for no other reason, this
 *      mapping of an actid to an auth-info is an artifact of the NCA
 *      DG protocol.  (There is no free space in the DG packet header,
 *      so the actid is "overloaded" to allow the server to determine
 *      the client's auth-info.)
 * 
 *      A single auth-info may be bound to any number of NCS binding
 *      handles (handle_t) subject to the auth package and application
 *      restrictions.  (I.e., the authentication package decides whether
 *      the same auth-info can be used for multiple servers that happen
 *      to have the same principal ID.)
 *
 *      Servers cache state across RPCs based on actid to lessen the
 *      need for the server to callback a client to validate the RPC
 *      sequence number or learn the auth session key.
 * 
 *      For clients to take advantage of server caching, they are
 *      encouraged to reuse actids on subsequent RPCs to a server that
 *      already knows that actid.
 *
 * Subtlety relating to server boot time
 * -------------------------------------
 *
 * Clients who don't (yet) know a server's boot time (i.e., whose binding
 * handle ".server_boot" time is zero) MUST generate a new CCTE/actid
 * before making a call.  Doing so ensures that the server does a WAY
 * callback to the client and that the client acquires the boot time.
 * The problem case we're trying to avoid is as follows:
 * 
 * - Client creates a binding to a server and calls it.
 * - The server calls back to client (WAY) causing the binding to acquire 
 *   the server's boot time.  
 * - The client creates a NEW binding to the same server and calls it
 *   using the same actid as in previous call.
 *
 * Since the server already knows about this actid (and sequence number)
 * it does NOT call back and the new binding doesn't acquire the server's
 * boot time.  This can cause protocol problems (inability to detect
 * a duplicate WAY from a rebooted instance of a server that's already
 * executed our call once).  (See "conv_who_are_you" in "conv.c" for
 * background.)
 * 
 * What's going on here is that on the one hand we're trying to minimize
 * the number of actids we create (in an attempt to minimize the number
 * of WAY callbacks that happen) and we can't (easily) keep track of
 * the boot times of all the servers which we've called with each actid,
 * while on the other hand, we MUST know the server boot time to maintain
 * "at most once" semantics.  You might imagine that a CCTE could hold
 * a list of boot times (keyed by server location) and that when you
 * picked up a CCTE, you'd stick the right boot time from its list into
 * the binding handle, but this would be complicated (if it works at all!).
 * We take the simpler approach of forcing new CCTE/actids to be created
 * for each binding.  This results in an excess of CCTEs, but they can
 * be put to good effect later (e.g., in parallel calls being made using
 * a single binding).
 *
 * Another fallout from this approach is that when a first call on a
 * binding gets a comm failure (i.e., times out), there are cases where
 * we can KNOW that the server didn't execute the call (i.e., in case
 * the call was to a non-idem procedure and the boot time in the binding
 * is zero--i.e., the server didn't call us back).  This turns out to
 * be useful since when a client has multiple bindings to some sort of
 * replicated service, there are (more) cases where it knows that it's
 * safe to move onto another binding (i.e., without accidentally executing
 * the call more than once).  We expect this to be a common occurrence
 * (e.g., in network partitions where some of the bindings you have are
 * to unreachable servers).
 */

typedef struct rpc_dg_cct_elt_t {
    struct rpc_dg_cct_elt_t *next;      /* -> next in hash chain */
    rpc_auth_info_p_t auth_info;        /* auth info to be used with server */
    rpc_key_info_p_t key_info;       /* key to use */
    struct rpc_dg_auth_epv_t *auth_epv; /* auth epv */
    dce_uuid_t actid;                       /* activity ID to use in msgs to server */
    unsigned32 actid_hash;              /* dce_uuid_hash(.actid) */
    unsigned32 seq;                     /* seq # to use in next call to server */
    rpc_clock_t timestamp;              /* last time connection used in a call */
    unsigned8 refcnt;                   /* # of references to the CCTE */
} rpc_dg_cct_elt_t, *rpc_dg_cct_elt_p_t;

#define RPC_DG_CCT_SIZE 29              /* Must be prime */

typedef struct rpc_dg_cct_t {
    unsigned32 gc_count;
    struct {
        rpc_dg_cct_elt_p_t first;
        rpc_dg_cct_elt_p_t last;
    } t[RPC_DG_CCT_SIZE]; 
} rpc_dg_cct_t;

/*
 * Client Connection Table Element Soft Reference.
 *
 * A "soft reference" is a reference that can cope with the fact that
 * the thing pointed to might be garbage collected (i.e., freed).  The
 * actual pointer in a soft reference is accompanied by a the "GC count"
 * for the table the pointee belongs to.  When any entry in the table
 * is GC'd, the GC count of the table is incremented.  The actual pointer
 * is considered valid iff the soft ref's GC count (which was the table
 * GC count at the time the soft ref was created) matches the table's
 * current GC count.
 *
 * CCT soft refs are used by call handles to refer to CCTEs that are
 * not in use (and hence are GC-able).
 */

typedef struct rpc_dg_cct_elt_ref_t {
    struct rpc_dg_cct_elt_t *ccte;  /* soft -> CCTE */
    unsigned32 gc_count;            /* CCT GC ref count for above pointer */
} rpc_dg_ccte_ref_t, *rpc_dg_ccte_ref_p_t;

/* ========================================================================= */

/*
 * Call handle types.
 */

/* ========================================================================= */

/*
 * Call states.
 *
 * For both client and server call handles, the only time that a handle
 * isn't actively performing or associated with a specific call is when
 * the handle is in the "idle" state.
 *
 * Client Call Handle State Transitions:
 *
 *                                                   end_call()
 *  start_call()                   transceive()      non-idempotent or
 *  handle        transceive()     non-maybe         idempotent w/large outs
 *  created       transmit()       call              call
 *    |   +------+         +------+         +------+         +------+
 *    +-->| init |-------->| xmit |-------->| recv |-------->|final |---+     
 *        +------+         +------+         +------+         +------+   |
 *         ^  ^           / |end_call() end_call()| \           |       |
 *         |  |          /  |quit             quit|  \          |       |
 *         |  |         /   |       +------+      |   \         |       |
 *         |  |        /    +------>|orphan|<-----+    \        |       |
 *         |  |       /             +------+            \       |       |
 *         |  |       \                | end_call()     /       |       |
 *         |  |        \               V quack signal  /        |       |
 *         |  |         \           +------+          /         |       |
 *         |  |          ---------->| idle |<---------          |       |
 *         |  |          end call() +------+  end call()        |       |
 *         |  |          maybe call   |  ^    idempotent call   |       |
 *         |  |                       |  |                      |       |
 *         |  +-----------------------+  +----------------------+       |
 *         |        start call()                     ack sent           |
 *         +------------------------------------------------------------+
 *                             start_call() - new call 
 * 
 * 
 * Server Call Handle State Transitions:
 *
 *                                                   stub returns
 *  do_request()   execute_call()                    non-idempotent or
 *  handle         stub is                           idempotent w/large outs
 *  created        being called    transmit()        call
 *    |   +------+(*)      +------+         +------+         +------+
 *    +-->| init |-------->| recv |-------->| xmit |-------->|final |---+     
 *        +------+         +------+         +------+         +------+   |
 *         ^  ^ | WAY     / |quit rcvd   quit rcvd| \           |       |
 *         |  | | rejects/  |                     |  \          |       |
 *         |  | | call  /   |       +------+(*)   |   \         |       |
 *         |  | |      /    +------>|orphan|<-----+    \        |       |
 *         |  | |     /             +------+            \       |       |
 *         |  | |     \                                 /       |       |
 *         |  | |      \                               /        |       |
 *         |  | |       \           +------+          /         |       |
 *         |  | |        ---------->| idle |<---------          |       |
 *         |  | |    stub returns   +------+   stub returns     |       |
 *         |  | |      maybe call    ^ |  ^    idempotent call  |       |
 *         |  | +--------------------+ |  +---------------------+       |
 *         |  +-------------------------             ack rcvd           |
 *         |        new call rcvd                                       |
 *         +------------------------------------------------------------+
 *                              new call rcvd  
 * 
 * (*) Server calls leave the "orphan" state by exiting.  
 */
typedef enum {
    rpc_e_dg_cs_init,       /* call initialized and in use */
    rpc_e_dg_cs_xmit,       /* call in use; currently sending data */
    rpc_e_dg_cs_recv,       /* call in use; awaiting data */
    rpc_e_dg_cs_final,      /* call in use; delayed ack pending */
    rpc_e_dg_cs_idle,       /* call NOT in use */
    rpc_e_dg_cs_orphan      /* call orphaned; waiting to exit */
} rpc_dg_call_state_t;

/* ========================================================================= */

/*
 * Call handle common header (CALL).
 *
 * Defines data that is common to client and server call handles
 * (see below).  Note that we're using the transmit queue's prototype
 * packet header as the repository for various per-call state (e.g. seq
 * #, interface ID).
 * 
 * The mutex (".mutex") protects fields in the composite call handles
 * (i.e., CCALLs and SCALLs).  The call handle mutex is a level 2 lock.
 * 
 * The condition variable (".cv") is used to wait for changes in the
 * call handle (typically, changes to the transmit and receive queues)
 * and is partnered with the handle's mutex.
 *        
 * The high_seq field is used to keep track of the sequence number space
 * in the event of callbacks.  This can get updated in two ways.  First,
 * by receiving a response with a bumped-up sequence number, as in the
 * 1.5 code.  Second, by receiving a request packet on the current activity
 * ID.  The second form will help avoid problems with having a callback
 * die (timeout, etc) and not receiving the results from the original
 * call;  in this case, we will still begin a new call with the appropriate
 * sequence number.
 *
 * The reference count (".refcnt") keeps track of all references to the
 * call handle.  For client call handles, references can be from:
 *
 *      - Client binding handles (cached client call handles).
 *      - The CCALLT.
 *      - The application thread (via the return from "start call").
 *      - The call handle's timer.
 *
 * Almost all fields are protected by the call handle's lock.  Two
 * exceptions: (1) the ".next" field, which is essentially owned and
 * protected by the data structure upon which the call handle is chained
 * (i.e. the binding handle); (2) the ".is_in_pkt_chain" flag, which
 * is owned and protected by the packet pool logic (and is therefore
 * wrapped with "unsigned : 0"s to (hopefully) force alignment).
 *
 * All call handle lock, condition wait and signalling operations should
 * be done via the macros below.
 */

typedef struct rpc_dg_call_t {
    rpc_call_rep_t c;                   /* call handle common header */
    struct rpc_dg_call_t *next;         /* -> next in chain */
    rpc_dg_call_state_t state;          /* call state */
    unsigned32 status;                  /* call's error status indicator / value */
    rpc_clock_t state_timestamp;        /* time we entered the current state */
    rpc_cond_t cv;                      /* conditional variable (->c.m is mutex) */
    rpc_dg_xmitq_t xq;                  /* transmit queue */
    rpc_dg_recvq_t rq;                  /* receive queue */
    struct rpc_dg_sock_pool_elt_t *sock_ref;   /* reference to socket pool element */
    unsigned32 actid_hash;              /* dce_uuid_hash(.call_actid) */
    rpc_key_info_p_t key_info;       /* key info */
    struct rpc_dg_auth_epv_t *auth_epv; /* auth epv */
    rpc_addr_p_t addr;                  /* location (network address) of peer */
    rpc_timer_t timer;                  /* timer */
    rpc_clock_t last_rcv_timestamp;     /* last time we rcv'd some packets for this call */
    rpc_clock_t start_time;             /* time call started */
    unsigned32 high_seq;                /* seq # from response packets or callback-generated call handles */
    struct rpc_dg_call_t *pkt_chain;    /* linked list used when waiting for an XQE. */
    unsigned32 com_timeout_knob;        /* com timeout control knob */
    dcethread* thread_id;               /* iff client is using a private socket*/
    unsigned8 refcnt;                   /* # of references to the call */
    unsigned8 n_resvs;                  /* # of packets reserved */
    unsigned8 n_resvs_wait;             /* # of packets waiting */
    unsigned8 max_resvs;                /* max # of pkts possibly reserved */
    unsigned blocked_in_receive: 1;     /* private socket users only */
    unsigned priv_cond_signal: 1;       /* T => call is being woken from recvfrom */
    unsigned stop_timer: 1;             /* T => timer should self-destruct at next execution */
    unsigned is_cbk: 1;                 /* T => this call handle was created specifically for a callback */
    unsigned : 0;                       /* force alignment */
    unsigned is_in_pkt_chain: 1;        /* T => waiting for an XQE to become available (see comments above!) */
    unsigned : 0;                       /* force alignment */
} rpc_dg_call_t, *rpc_dg_call_p_t;

/*
 * Logically these fields are part of the CALL, but they live in the
 * transmit queue's prototype packet header as an optimization.
 */

#define call_actid          xq.hdr.actuid
#define call_object         xq.hdr.object
#define call_if_id          xq.hdr.if_id
#define call_if_vers        xq.hdr.if_vers
#define call_ihint          xq.hdr.ihint
#define call_ahint          xq.hdr.ahint
#define call_opnum          xq.hdr.opnum
#define call_seq            xq.hdr.seq
#define call_server_boot    xq.hdr.server_boot

/*
 * The various operations on the call handle mutex and condition variables.
 */

#define RPC_DG_CALL_LOCK_INIT(call)     RPC_CALL_LOCK_INIT(&(call)->c)
#define RPC_DG_CALL_LOCK(call)          RPC_CALL_LOCK(&(call)->c)
#define RPC_DG_CALL_UNLOCK(call)        RPC_CALL_UNLOCK(&(call)->c)
#define RPC_DG_CALL_TRY_LOCK(call,bp)   RPC_CALL_TRY_LOCK(&(call)->c,(bp))
#define RPC_DG_CALL_LOCK_DELETE(call)   RPC_CALL_LOCK_DELETE(&(call)->c)
#define RPC_DG_CALL_LOCK_ASSERT(call)   RPC_CALL_LOCK_ASSERT(&(call)->c)
#define RPC_DG_CALL_UNLOCK_ASSERT(call) RPC_CALL_UNLOCKED_ASSERT(&(call)->c)

#define RPC_DG_CALL_COND_INIT(call)     RPC_COND_INIT((call)->cv, (call)->c.m)
#define RPC_DG_CALL_COND_DELETE(call)   RPC_COND_DELETE((call)->cv, (call)->c.m)
#define RPC_DG_CALL_COND_WAIT(call)     RPC_COND_WAIT((call)->cv, (call)->c.m)
#define RPC_DG_CALL_COND_SIGNAL(call)   RPC_COND_SIGNAL((call)->cv, (call)->c.m)
                                             
#define RPC_DG_CALL_IS_SERVER(call)     RPC_CALL_IS_SERVER(&(call)->c)
#define RPC_DG_CALL_IS_CLIENT(call)     RPC_CALL_IS_CLIENT(&(call)->c)     

/*
 * A common function for changing the call handle state.
 */

#define RPC_DG_CALL_SET_STATE(call, s) { \
    if ((call)->state != (s)) { \
        RPC_DBG_PRINTF(rpc_e_dbg_dg_state, 2, ("(RPC_DG_CALL_SET_STATE) " #call "=%08x, old state=%u, new state=%u\n", \
                        (call), (int) (call)->state, (int) (s))); \
        (call)->state = (s); \
        (call)->state_timestamp = rpc__clock_stamp(); \
    } \
}
    
/* ========================================================================= */

/*
 * Server call handle (SCALL).
 *
 * Defines the dg-specific, server-specific structure referred to 
 * from an rpc_call_rep_t.
 *
 * To support normal callbacks, an SCALL is linked with its paired CCALL
 * (a CCALL with the same actid as the SCALL) via the cbk_ccall field.
 *
 * .has_call_executor_ref means that one of the scall's referece counts
 * is for the rpc__dg_execute_call() processing.  This flag was introduced
 * for orphan processing since orphaned scalls can't have their final cleanup
 * performed until the call executor processing has completed.
 * 
 * Callbacks
 * --------
 *
 * Callback CCALLs linger around (cached on the paired scall) until the
 * paired scall processing frees them.
 * 
 * The following shows the relationships among the SCT, CCALLs, and SCALLs
 * in the case of callback processing in the original server process
 * (i.e. the process where the server manager is calling back to the
 * original client; where the callback is originating).  Note the values
 * of the {s,c}call fields in this picture and how the differ from the
 * subsequent picture; this is critical to understanding callback
 * relationships.
 *
 * This is only the original server process half of a callback.  Refer
 * to the following description of CCALLs for the special relationships
 * that exist for a SCALL that is created to perform a callback.
 *
 *        SCT
 *  +------+------+
 *  |      |      |
 *  |-------------|             SCALL
 *  |  U1  |  ----|------> +--------------+ <---------------------+
 *  |-------------| <---+  | .c.is_cbk = F|                       |
 *  |      |      |     |  |--------------|                       |
 *  |-------------|     +--|-- .scte      |          is_cbk       |
 *  |      |      |        |--------------|          CCALL        |
 *  |-------------|        | .cbk_ccall --|---> +--------------+  |
 *  |      |      |        +--------------+     | .c.is_cbk = T|  |
 *  |-------------|                             |--------------|  |
 *  |      |      |                             | .ccte_ref = 0|  |
 *  +-------------+                             |--------------|  |
 *                                              | .cbk_scall --|--+
 *                                              +--------------+
 *
 * See the subsequent CCALL description for more info on callbacks
 * (ESPECIALLY THE LOCKING HIERARCHY).
 *
 * All fields are protected by the common call handle lock.
 */

typedef struct rpc_dg_scall_t {
    rpc_dg_call_t c;                    /* call handle common header */
    rpc_dg_recvq_elt_p_t fwd2_rqe;  /* if non-NULL, -> 1st half of 2-part forwarded pkt */
    struct rpc_dg_sct_elt_t *scte;
    struct rpc_dg_ccall_t *cbk_ccall;   /* -> callback paired ccall */
    struct rpc_dg_binding_server_t *h;  /* binding handle associated with this call */
    unsigned client_needs_sboot: 1;     /* T => client needs to be WAY'd to get it the server boot time */
    unsigned call_is_setup: 1;          /* T => "call executor" has been established */
    unsigned has_call_executor_ref: 1;  /* T => "call executor" has ref */
    unsigned call_is_queued: 1;         /* T => call has been queued */
} rpc_dg_scall_t, *rpc_dg_scall_p_t;

/* ========================================================================= */

/*
 * Client call handle (CCALL).
 *
 * Defines the dg-specific, client-specific structure referred to 
 * from an rpc_call_rep_t.
 *
 * CCALLs are entered into the CCALL table (CCALLT); see below.
 *
 * A call handle is associated with both a single binding handle and
 * a single CCTE.  In addition to providing the data structures for a
 * in-progress call, a call handle provides a inter-RPC cache of useful
 * information to allow us to quickly start up subsequent RPCs.  Changes
 * to the state of the binding handle (e.g. object id, server location)
 * require that the call handle be appropriatly updated.  On every call,
 * the call handle's per call state (e.g. interface id, ihint, opn) must
 * be properly initialized.
 * 
 * An in-progress call increments its associated CCTE's reference count
 * for the duration of the call (thereby reserving the CCTE's actid).
 * The reference count should not be decremented until the call no longer
 * requires the exclusive use of that actid.
 * 
 * A call handle may be re-associated with a new CCTE (if the last one
 * it used happens to be "inuse" or has been freed and a call is starting
 * up).  If such a re-association occurs, cached activity related info in
 * the call handle (e.g., the actid and the ahint) need to be reinitialized.
 *
 * Callbacks 
 * ---------
 *
 * The server call handle (".cbk_scall") refers to the handle of a callback
 * from the remote server to the local client.  Callbacks are indicated
 * by the receipt of request packet on a socket being used by a client;
 * the activity ID in the request must match the activity ID being used
 * by the client making the call.  (Requests packets with non-matching
 * activity IDs received on client sockets are discarded.)
 * 
 * When a callback request is received, the request packet handling code
 * sets ".cbk_start" and awakens the application client thread to let
 * it know to start executing the callback.
 * 
 * Callback SCALLs linger around for a while (cached on the paired ccall)
 * in the case of a non-idempotent callback that is awaiting a response
 * ack.  The callback SCALL is freed when the original call completes.
 * 
 * The following shows the relationships among the CCALLT, CCALLs, and
 * SCALLs in the case of callback processing in the original client
 * processing (i.e. where the callback manager is executing).  Note the
 * values of the {s,c}call fields in this picture and how the differ
 * from the previous picture; this is critical to understanding callback
 * relationships.
 *
 * This is only the original client process half of a callback.  Refer
 * to the above description of SCALLs for the special relationships that
 * exist for a CCALL that is created to perform a callback.
 *
 *      CCALLT
 *  +------+------+
 *  |      |      |
 *  |-------------|             CCALL
 *  |  U1  |  ----|------> +--------------+ <---------------------+
 *  |-------------| <---+  | .c.is_cbk = F|                       |
 *  |      |      |     |  |--------------|                       |
 *  |-------------|     +--|-- .ccte_ref  |          is_cbk       |
 *  |      |      |        |--------------|          SCALL        |
 *  |-------------|        | .cbk_scall --|---> +--------------+  |
 *  |      |      |        +--------------+     | .c.is_cbk = T|  |
 *  |-------------|                             |--------------|  |
 *  |      |      |                             | .scte = NULL |  |
 *  +-------------+                             |--------------|  |
 *                                              | .cbk_ccall --|--+
 *                                              +--------------+
 *
 * Note the special case of the "conv_who_are_you" (WAY) callback used
 * to do "connection management".  For protocol correctness reasons,
 * the request packet in this callback does not have the same activity
 * ID as the client.  We handle this case specially (no cbk scall is used); 
 * see comments above "rpc__dg_handle_conv" for details.
 * 
 *
 * Proxy Calls
 * -----------
 *
 * Proxy calls are calls from a server manager during the execution of
 * an "originating" call that are performed using the client's activity
 * id (hence sequence number space).  Callbacks as described above are
 * one form of proxy calls.  The sequence number from the server's response
 * packet header tells us the highest sequence number it consumed doing
 * proxy calls is saved (".high_seq").  This enables us to properly
 * manage this activities sequence number space in the face of such calls.
 * When the original call completes, the value of the response sequence
 * number is stored back into the CCTE.
 * 
 * All fields are protected by the common call handle lock.
 *
 * LOCKING HIERARCHY:
 *  The only occasion where more than one call handle may require locking
 *  at any instant is due to callbacks.  The locking hierarchy for callback
 *  ccall / scall pairs depends on which side of the wire you're on (which
 *  really means which call of the pair has been tagged ".is_cbk == true").
 *  The reason for the difference is for convienience to the listener
 *  processing.  In the original client process, the listener will have
 *  a ccall and need to get the associated is_cbk scall.  In the original
 *  server process the listener will have a scall and will need to get
 *  the associated is_cbk ccall.  See the above for a visual description.
 *  
 *  So, the locking hierarchy order is (first, second):
 *      ccall, is_cbk scall
 *      scall, is_cbk ccall
 */

/*
 * Ping information (part of CCALL).
 */

typedef struct rpc_dg_ping_info_t {
    rpc_clock_t next_time;          /* time to send next ping */
    rpc_clock_t start_time;         /* time we started pinging */
    unsigned16 count;               /* # of unack'd pings sent in a row */
    unsigned pinging: 1;            /* T => ping has been sent but not ack'd */
} rpc_dg_ping_info_t;

#define RPC_C_DG_FIRST_PING_INTERVAL    RPC_CLOCK_SEC(2)
#define RPC_C_DG_MAX_INTER_PING_TIME    RPC_CLOCK_SEC(8)

#define RPC_DG_PING_INFO_INIT(ping) \
{ \
    (ping)->pinging   = false; \
    (ping)->next_time = rpc__clock_stamp() + RPC_C_DG_FIRST_PING_INTERVAL; \
}

#define RPC_PING_INFO_NEXT(ping, now) \
{ \
    (ping)->next_time = (now) + MIN(RPC_C_DG_MAX_INTER_PING_TIME, RPC_C_DG_FIRST_PING_INTERVAL << (ping)->count); \
    (ping)->count++; \
}

/* 
 * Quit information (part of CCALL).
 */

typedef struct rpc_dg_quit_info_t {
    rpc_clock_t next_time;          /* time to send next quit */
    unsigned quack_rcvd: 1;         /* quack received notification flag */
} rpc_dg_quit_info_t;

/* 
 * Cancel information (part of CCALL).
 */

typedef struct rpc_dg_cancel_info_t {
    rpc_clock_t next_time;          /* time to (re)send cancel request */
    unsigned16 local_count;         /* # of detected local cancel events */
    unsigned16 server_count;        /* # of server-accepted cancel events */
    rpc_clock_t timeout_time;       /* when to give up */
    unsigned server_is_accepting: 1;
    unsigned server_had_pending: 1;
} rpc_dg_cancel_info_t;

/*
 * The CCALL itself.
 */

typedef struct rpc_dg_ccall_t {
    rpc_dg_call_t c;                /* call handle common header */
    rpc_dg_recvq_elt_p_t fault_rqe; /* received call fault rqe */
    unsigned32 reject_status;       /* if non-0, then value rcv'd in a reject pkt */
    unsigned cbk_start: 1;          /* T => awakened to start a callback */
    unsigned response_info_updated: 1; /* T => ahint, etc.. has been updated */
    unsigned server_bound: 1;       /* T => "real" server address is known */
    rpc_dg_scall_p_t cbk_scall;     /* -> callback paired scall */
    rpc_dg_ccte_ref_t ccte_ref;     /* a soft ref to the associated CCTE */
    struct rpc_dg_binding_client_t *h;  /* binding handle that started this call */
    rpc_dg_ping_info_t ping;        /* ping information */
    rpc_dg_quit_info_t quit;        /* quit information */
    rpc_dg_cancel_info_t cancel;    /* cancel information */
    rpc_clock_t timeout_stamp;      /* max call execution time */
    unsigned_char_p_t auth_way_info;/* ptr to oversized auth PAC buffer */
    unsigned32 auth_way_info_len;   /* length of preceding buffer */
} rpc_dg_ccall_t, *rpc_dg_ccall_p_t;

/* ========================================================================= */

/*
 * Client call handle table (CCALLT).
 *
 * A mapping from activity IDs (the keys) to client calls.  The CCALLT
 * is a hash table with separate chaining.  The CCALLT is used by the
 * packet listen loop to dispatch incoming packets received on client
 * connection sockets to the appropriate call.  The CCALLT is a hash
 * table with separate chaining.
 *
 * In the case of callback related packets (i.e. client -> server pkts),
 * the ahint in the packet header may be used as an index into this table.
 *
 * See "dgglob.h" for definition of the table itself.
 *
 * The CCALLT (scanning, inserting, deleting) is protected by the global lock.
 */

#define RPC_DG_CCALLT_SIZE 29           /* Must be prime */

/* ======================================================================== */

/*
 * Server connection table (SCT)
 *
 * A mapping from remote client activity IDs (the keys) to server calls.
 * The SCT is a hash table with separate chaining.  The SCT is used by
 * the packet listen loop to dispatch incoming packets received on server
 * connection sockets to the appropriate server call handle.  Additionally
 * the SCT holds "inter-call history" about remote clients (seq and
 * auth_info).
 * 
 * The SCT is roughly the server-side analog to the CCALLT and CCT.  It's
 * like the CCALLT in that it's the database for demultiplexing packets
 * to call handles based on the packet's activity ID.  It's like the
 * CCT in that it maintains inter-call history.  We maintain this
 * information as a single table on the server side so that we can deal
 * with the case of request from previously unknown clients without having
 * to do lookups/inserts into two tables.  (The client being the initiator
 * of calls doesn't have the problem of dealing with packets arriving
 * from unknown servers.)
 *
 * The ".ahint" field in the packet header of packets sent from client
 * to server (i.e. NOT callback) is an index into the SCT.
 *
 * Logically, a "server" maintains some RPC sequence number information
 * for a connection to model the client's sequence number state; this
 * is done to allow the server to detect/prevent reruns of non-idempotent
 * RPCs.  The server is only truely synchronized with the client when
 * it performs a WAY to definitively learn the client's current sequence
 * number.  In all other instances the model is only approximate due
 * to the fact that the server doesn't necessarily see all RPCs the client
 * makes (nor is the client obgligated to only increase the seq by 1
 * on each succesive call).
 *
 * At any instant, the SCTE ".high_seq" represents the *highest* RPC
 * seq of the following: "tentatively accepted" RPCs for execution by
 * the server, RPCs actually executed by the server or the result of
 * a WAY.  The creation of a SCTE occurs because a request pkt for a
 * connection is received and the server knows nothing about the
 * connection.  A created SCTE's ".high_seq" is initialized to the
 * (inducing_request_seq - 1); logically the highest previous call that
 * the server could have known about.
 *
 * A call may be tentatively accepted as long as it's sequence number
 * is greater than the SCTE ".high_seq"; this new sequence becomes
 * the SCTE ".high_seq".  The state of the SCTE ".high_seq" is represented
 * by ".high_seq_is_way_validated".  Initially ".high_seq_is_way_validated"
 * is false; it is set to true once a WAY is performed to the client.
 * A WAY validated SCTE ".high_seq" must be used for comparison prior
 * the actual execution (stub dispatch) of a tentatively accepted
 * Non-idempotent RPC.
 *
 * Note that in the case of maybe-calls, it is not necessary for the 
 * call's sequence number to be greater that the SCTE ".high_seq", since
 * the call's request packet could have been (legally) reordered wrt a
 * packet from a subsequent call; in this case the maybe call will not
 * update the ".high_seq" field.
 *
 * The SCTE ".scall" field points to a SCALL for the connection.  This
 * field is used both to locate the SCALL for an in-progress call and
 * as a place to cache a previously created / used SCALL.  If the SCALL
 * ".call_seq" field matches the SCTE ".high_seq" field then the SCALL
 * is for the connection's current (or just completed) call; otherwise
 * the SCALL is just being cached and logically does not contain useful
 * information.
 *
 * The SCTE ".maybe_chain" field points to a list of scalls which represent
 * currently/recently executing calls made using the "maybe" attribute.
 * Since the client does not wait for such calls to complete before
 * continuing, it's perfectly legal to have several of maybe calls executing
 * in paralel.
 *
 * See the dgsct.[ch] for the operations available for the SCT.
 * 
 * Each SCTE is reference counted.  The SCALLs reference to the SCTE
 * counts as a reference.  Lookup and get functions increment the ref
 * count since they are returning a reference.  The SCT reference to the
 * SCTE DOES count as a reference.
 *
 * The SCT is garbage collectable -- SCTEs that are not "inuse" (i.e.
 * refcnt == 1; only referenced by the SCT) can be freed.
 * 
 * See "dgglob.h" for definition of the table itself.
 *
 * The SCT and all it's elements are protected by the global lock.
 */

typedef struct rpc_dg_sct_elt_t {
    struct rpc_dg_sct_elt_t *next;      /* -> next in hash chain */
    dce_uuid_t actid;                       /* activity of of remote client */
    unsigned16 ahint;                   /* dce_uuid_hash(.actid) % RPC_DG_SCT_SIZE */
    unsigned32 high_seq;                /* highest known seq */
    unsigned high_seq_is_way_validated: 1; /* T => .high_seq is WAY validated */
    unsigned8 refcnt;                   /* scte reference count */
    rpc_key_info_p_t key_info;       /* key info to be used */
    struct rpc_dg_auth_epv_t *auth_epv; /* auth epv */
    rpc_dg_scall_p_t scall;             /* -> server call handle being used in call */
    rpc_dg_scall_p_t maybe_chain;       /* list of maybe calls associated with */
                                        /* this connection.                    */
    rpc_clock_t timestamp;              /* last time connection used in a call */
    rpc_dg_client_rep_p_t client;       /* -> to client handle   */
} rpc_dg_sct_elt_t, *rpc_dg_sct_elt_p_t;

#define RPC_DG_SCT_SIZE 101             /* Must be prime */

/* ========================================================================= */

/*
 * Data types that describe the DG protocol service binding rep.
 */

typedef struct rpc_dg_binding_rep_t {    rpc_binding_rep_t c;
} rpc_dg_binding_rep_t, *rpc_dg_binding_rep_p_t;

typedef struct rpc_dg_binding_client_t {
    rpc_dg_binding_rep_t c;
    rpc_dg_ccall_p_t ccall;
    unsigned32 server_boot;
    struct rpc_dg_binding_server_t *shand;     /* => to server handle if doing callback */
    rpc_binding_rep_p_t maint_binding;
    unsigned8 is_WAY_binding;   /* number of packets reserved
                                 * non-zero if handle is for doing a WAY or WAY2 */
    unsigned host_bound: 1;            /* compat only; T => if contains a valid netaddr */
} rpc_dg_binding_client_t, *rpc_dg_binding_client_p_t;

typedef struct rpc_dg_binding_server_t {
    rpc_dg_binding_rep_t c;
    rpc_dg_binding_client_p_t chand;   /* => to client binding handle for callbacks */
    rpc_dg_scall_p_t scall;
} rpc_dg_binding_server_t, *rpc_dg_binding_server_p_t;

/* ========================================================================= */

/*
 * Local fragment size and packet buffer.
 *
 */

#define RPC_DG_FRAG_SIZE_TO_NUM_PKTS(frag_size,n_pkts) \
{ \
    (n_pkts) = ((frag_size) - RPC_C_DG_RAW_PKT_HDR_SIZE) \
             / RPC_C_DG_MAX_PKT_BODY_SIZE; \
    if (((frag_size) - RPC_C_DG_RAW_PKT_HDR_SIZE) > \
        ((n_pkts) * RPC_C_DG_MAX_PKT_BODY_SIZE)) \
        (n_pkts)++; \
}

#define RPC_DG_NUM_PKTS_TO_FRAG_SIZE(n_pkts,frag_size) \
{ \
    (frag_size) = ((n_pkts) * RPC_C_DG_MAX_PKT_BODY_SIZE \
                   + RPC_C_DG_RAW_PKT_HDR_SIZE) & ~0x7; \
}

/*
 * Max receive fragment size
 *
 * This is the lower bound on the size of a single fragment
 * (including RPC packet header and body, i.e., PDU) that all
 * implementations must be able to receive. It's defined by the
 * protocol.
 * (Derived from RPC_C_IP_UDP_MAX_LOC_UNFRG_TPDU.)
 */

#define RPC_C_DG_MUST_RECV_FRAG_SIZE 1464

/*
 * RPC_C_DG_MAX_PKT_SIZE must be larger than (or equal to)
 * RPC_C_DG_MUST_RECV_FRAG_SIZE so that the minimum packet
 * reservation becomes 1.
 */
#if RPC_C_DG_MUST_RECV_FRAG_SIZE > RPC_C_DG_MAX_PKT_SIZE
#error RPC_C_DG_MAX_PKT_SIZE is less than RPC_C_DG_MUST_RECV_FRAG_SIZE.
#endif

/*
 * Max fragment size
 *
 * This is the size of the largest RPC fragment (including RPC
 * packet header and body, i.e., PDU) we can cope with. This is an
 * IMPLEMENTATION artifact and does not represent the largest
 * fragment the protocol supports. We need this to determine the
 * size of some resources at compile-time.
 */

#ifndef RPC_C_DG_MAX_FRAG_SIZE

#define RPC_C_DG_MAX_FRAG_SIZE \
    (2 * RPC_C_DG_MAX_PKT_BODY_SIZE + RPC_C_DG_RAW_PKT_HDR_SIZE)
#define RPC_C_DG_MAX_NUM_PKTS_IN_FRAG   2

#else   /* RPC_C_DG_MAX_FRAG_SIZE */

#if ((RPC_C_DG_MAX_FRAG_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE) \
     / RPC_C_DG_MAX_PKT_BODY_SIZE) * RPC_C_DG_MAX_PKT_BODY_SIZE == \
    RPC_C_DG_MAX_FRAG_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE

#define RPC_C_DG_MAX_NUM_PKTS_IN_FRAG \
    ((RPC_C_DG_MAX_FRAG_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE) \
        / RPC_C_DG_MAX_PKT_BODY_SIZE)

#else

#define RPC_C_DG_MAX_NUM_PKTS_IN_FRAG \
    (((RPC_C_DG_MAX_FRAG_SIZE - RPC_C_DG_RAW_PKT_HDR_SIZE) \
        / RPC_C_DG_MAX_PKT_BODY_SIZE) + 1)

#endif

#endif  /* RPC_C_DG_MAX_FRAG_SIZE */

#if RPC_C_DG_MAX_NUM_PKTS_IN_FRAG > (RPC_C_MAX_IOVEC_LEN - 1)
#error RPC_C_DG_MAX_NUM_PKTS_IN_FRAG greater than RPC_C_MAX_IOVEC_LEN!
#endif


/* =============================================================================== */

/*
 * Local window size and socket buffering.
 *
 * "RPC_C_DG_MAX_WINDOW_SIZE" is the size of the window we want to offer
 * (in kilobytes). The window size indicates the receiver's idea of
 * how much buffering is available in the network (including
 * IP/UDP). The window size is the receiver's advice on how much
 * *unacknowledged* data the sender should have outstanding. This is
 * changed dynamically by the receiver.
 * The amount of socket receive buffering required is the product of
 * the max window size, 1024,
 * and the socket's expected "load" (i.e., how many simultaneous calls
 * with what amounts of in/out data we expect a single socket to support).
 * We know the first two values; we don't know the last one.  We assert
 * it to be "RPC_C_DG_SOCK_LOAD" below.
 * Since the private socket's "load" is 1, it will be adjusted in
 * use_protseq().
 */

#ifndef RPC_C_DG_MAX_WINDOW_SIZE
#define RPC_C_DG_MAX_WINDOW_SIZE    24
#endif

#ifndef RPC_C_DG_SOCK_LOAD
#define RPC_C_DG_SOCK_LOAD          2
#endif

#define RPC_C_DG_MAX_WINDOW_SIZE_BYTES \
    (RPC_C_DG_MAX_WINDOW_SIZE * 1024)

#define RPC_C_DG_SOCK_DESIRED_RCVBUF \
    (RPC_C_DG_MAX_WINDOW_SIZE_BYTES * RPC_C_DG_SOCK_LOAD)

/*
 * A macro to convert a given socket receive buffering size into a window
 * size value we might offer.  This macro is used after we ask for a
 * particular socket receive buffering, inquire what the system actually
 * gave us, and need to figure out what window size to offer as a result.
 *
 * The most conservative thing to do would be:
 *
 *  WS = rcvbuf / "socket load" / 1024 ;
 *  WS = (WS > RPC_C_DG_MAX_WINDOW_SIZE)? RPC_C_DG_MAX_WINDOW_SIZE : WS;
 *
 * We don't really trust our "load" measure well enough, but have no
 * better alternative.
 * Unless there are large IN/OUTs, most fragments actually sent will
 * be less than max_frag_size and we may offer windows that are as
 * low as this might result in. On the other hand, our "load"
 * measure may be too small and we may offer too large windows.
 * They may be well-balanced...
 */

#define RPC_DG_RBUF_SIZE_TO_WINDOW_SIZE(rcvbuf, is_private, frag_size, window_size) \
{ \
    if ((is_private)) \
        (window_size) = (rcvbuf) >> 10; \
    else \
        (window_size) = ((rcvbuf) / RPC_C_DG_SOCK_LOAD) >> 10; \
    if ((window_size) > RPC_C_DG_MAX_WINDOW_SIZE) \
        (window_size) = RPC_C_DG_MAX_WINDOW_SIZE; \
}

/*
 * The amount of socket send buffering we establish is based on how much
 * window we expect to be offered from remote peers and the socket's
 * expected load (as described above).  Essentially, we guess that this
 * value is the same as the amount of receive buffering.
 */

#define RPC_C_DG_SOCK_DESIRED_SNDBUF \
    RPC_C_DG_SOCK_DESIRED_RCVBUF

/* ========================================================================= */

/*
 * Macros to decide if a given socket pool entry was created in service
 * of a server or a client.
 */

#define RPC_DG_SOCKET_IS_SERVER(sock_pool_elt)   ((sock_pool_elt)->is_server)
#define RPC_DG_SOCKET_IS_CLIENT(sock_pool_elt)   (! (sock_pool_elt)->is_server)

/* ========================================================================= */

/*
 * !!! DON'T USE THE rpc__socket API DIRECTLY !!!
 *
 * To centralize the chores of pkt logging and lossy mode behavior, the
 * following macros must be used by ALL DG code:
 *      RPC_DG_SOCKET_RECVFROM(sock, buf, buflen, from, ccp, serrp)
 *      RPC_DG_SOCKET_RECVFROM_OOL(sock, buf, buflen, from, ccp, serrp)
 *      RPC_DG_SOCKET_SENDMSG(sock, iov, iovlen, addr, ccp, serrp)
 *      RPC_DG_SOCKET_SENDMSG_OOL(sock, iov, iovlen, addr, ccp, serrp)
 *
 * Note: this DG API exactly corresponds to the semantics of the rpc__socket
 * interface.  As such, it deals in terms of "raw pkts" structures (i.e.
 * MISPACKED_HDR systems must do conversions before and after transmitting
 * and receiving respectively).  The single difference in the interfaces is that
 * these recvfrom macros expect a buf of type rpc_dg_raw_pkt_p_t (the socket
 * versions expect a byte_p_t).
 * 
 * Pkt receive processing is the same regardless of whether or not lossy
 * capability is present (lossy currently only affects pkt transmission).
 * 
 * If RPC_DG_PLOG is defined, pkt logging capability exists (if not compiled
 * for pkt logging capability the plog operations are a no-op; i.e. doesn't
 * even involve a subroutine call).  Note that even if RPC_DG_PLOG is
 * defined, nothing bad will happen unless the rpc_e_dbg_dg_plog debug
 * switch is set appropriately.  See "dgutl.c" for more info.
 * 
 * If RPC_DG_LOSSY is defined, lossy mode capability exists and all xmits
 * are mapped to the dg lossy xmit routine.  The lossy xmit routine
 * performs the necessary pkt logging.  It will also do bad stuff every
 * once in a while.  Used for testing.  Note that even if RPC_DG_LOSSY
 * is defined, nothing bad will happen unless the rpc_e_dbg_dg_lossy
 * debug switch is set appropriately.  See "dglossy.c" for more info.
 * 
 * Note that we don't (currently) use rpc__socket_sendto() (or
 * RPC_SOCKET_SENDTO) but just in case, we alias them here.
 */

#define RPC_DG_SOCKET_RECVFROM(sock, buf, buflen, from, ccp, serrp) \
    { \
        *(serrp) = rpc__socket_recvfrom(sock, (byte_p_t)(buf), buflen, from, ccp); \
        if (! RPC_SOCKET_IS_ERR(*serrp)) \
        { \
            RPC_DG_PLOG_RECVFROM_PKT(&(buf->hdr), &(buf->body)); \
        } \
    }

#define RPC_DG_SOCKET_RECVFROM_OOL(sock, buf, buflen, from, ccp, serrp) \
    { \
        *(serr) = rpc__socket_recvfrom(sock, (byte_p_t)(buf), buflen, from, ccp); \
        if (! RPC_SOCKET_IS_ERR(*serrp)) \
        { \
            RPC_DG_PLOG_RECVFROM_PKT(&(buf->hdr), &(buf->body)); \
        } \
    }

#ifndef RPC_DG_LOSSY
#  define RPC_DG_SOCKET_SENDMSG_OOL(sock, iov, iovlen, addr, ccp, serrp) \
        { \
            *(serrp) = rpc__socket_sendmsg(sock, iov, iovlen, addr, ccp); \
            if (! RPC_SOCKET_IS_ERR(*serrp)) \
            { \
                RPC_DG_PLOG_SENDMSG_PKT(iov, iovlen); \
            } \
        }

#  define RPC_DG_SOCKET_SENDMSG(sock, iov, iovlen, addr, ccp, serrp) \
        { \
            *(serrp) = rpc__socket_sendmsg(sock, iov, iovlen, addr, ccp); \
            if (! RPC_SOCKET_IS_ERR(*serrp)) \
            { \
                RPC_DG_PLOG_SENDMSG_PKT(iov, iovlen); \
            } \
        }

#  define RPC_DG_SOCKET_SENDTO_OOL  rpc__dg_socket_sendto_not_impl
#  define RPC_DG_SOCKET_SENDTO      rpc__dg_socket_sendto_not_impl

#else

#  define RPC_DG_SOCKET_SENDMSG_OOL(sock, iov, iovlen, addr, ccp, serrp) \
        { \
            *(serrp) = rpc__dg_lossy_socket_sendmsg(sock, iov, iovlen, addr, ccp); \
        }

#  define RPC_DG_SOCKET_SENDMSG(sock, iov, iovlen, addr, ccp, serrp) \
        { \
            *(serrp) = rpc__dg_lossy_socket_sendmsg(sock, iov, iovlen, addr, ccp); \
        }

#  define RPC_DG_SOCKET_SENDTO_OOL  rpc__dg_socket_sendto_not_impl
#  define RPC_DG_SOCKET_SENDTO      rpc__dg_socket_sendto_not_impl



#ifdef __cplusplus
extern "C" {
#endif


PRIVATE rpc_socket_error_t rpc__dg_lossy_socket_sendmsg (
        rpc_socket_t /*sock*/,
        rpc_socket_iovec_p_t /*iov*/,
        int /*iov_len*/,
        rpc_addr_p_t /*addr*/,
        int * /*cc*/
    );

#endif

#ifdef __cplusplus
}
#endif


/* ======================================================================== */

/*
 * A macro to align a pointer to a 0 mod 8 boundary.  The #ifndef is here
 * to allow a per-system override in case the way we do it here doesn't
 * work for some systems.
 */

#ifndef RPC_DG_ALIGN_8
#  define RPC_DG_ALIGN_8(p) (((unsigned long)((unsigned8 *) (p)) + 7) & ~7)
#endif

/* ========================================================================= */

/*
 * Authentication operations EPV.
 *
 * A pointer to one of these may be found through the auth_info
 * pointer in the SCTE and CCTE structures.
 *
 * The operations are invoked at the "right time" during call processing.
 *
 * EPVs are assumed to be read-only and shared; each authentication
 * module declares exactly one.
 *
 * The auth_data structures, on the other hand, are managed using
 * the reference and release operations.
 */

#define RPC_DG_AUTH_REFERENCE(info) \
    rpc__auth_info_reference(info)
    
#define RPC_DG_AUTH_RELEASE(info) \
    rpc__auth_info_release(&(info))

#define RPC_DG_KEY_REFERENCE(key) \
    rpc__key_info_reference(key)
    
#define RPC_DG_KEY_RELEASE(key) \
    rpc__key_info_release(&(key))

    
#ifdef __cplusplus
extern "C" {
#endif


/* 
 * invoked on server to create new auth_t for a new client 
 */
typedef rpc_key_info_p_t (*rpc_dg_auth_create_fn_t) (
        unsigned32 * /*st*/
    );

/* 
 * invoked before each call 
 */
typedef void (*rpc_dg_auth_pre_call_fn_t) (
        rpc_key_info_p_t                /*info*/,
        handle_t                         /*h*/,
        unsigned32                      * /*st*/
    );


typedef void (*rpc_dg_auth_encrypt_fn_t) (
        rpc_key_info_p_t                /*info*/,
        rpc_dg_xmitq_elt_p_t            ,
        unsigned32                      * /*st*/
    );

    
/* 
 * invoked after header is packed   
 */
typedef void (*rpc_dg_auth_pre_send_fn_t) (
        rpc_key_info_p_t                /*info*/,
        rpc_dg_xmitq_elt_p_t            ,
        rpc_dg_pkt_hdr_p_t              ,
        rpc_socket_iovec_p_t             /*iov*/,
        int                              /*iovlen*/,
        pointer_t                        /*cksum*/,
        unsigned32                      * /*st*/
    );

        

/* 
 * invoked after header in unpacked
 */
typedef void (*rpc_dg_auth_recv_ck_fn_t) (
        rpc_key_info_p_t                /*info*/,
        rpc_dg_recvq_elt_p_t             /*pkt*/,
        pointer_t                        /*cksum*/,
        unsigned32                      * /*st*/
    );

/* 
 * called on server side 
 */
typedef void (*rpc_dg_auth_way_fn_t) (
        rpc_key_info_p_t               /* in */   /*info*/,
        handle_t                        /* in */   /*h*/,
        dce_uuid_t                          /* in */  * /*actuid*/,
        unsigned32                      /* in */   /*boot_time*/,
        unsigned32                      /* out */ * /*seqno*/,
        dce_uuid_t                          /* out */ * /*cas_uuid*/,
        unsigned32                      /* out */ * /*st*/
    );

    
/* 
 * called on client side 
 */
typedef void (*rpc_dg_auth_way_handler_fn_t) (
        rpc_key_info_p_t                /*info*/,
        ndr_byte                        * /*in_data*/,
        signed32                         /*in_len*/,
        signed32                         /*out_max_len*/,
        ndr_byte                        ** /*out_data*/,
        signed32                        * /*out_len*/,
        unsigned32                      * /*st*/
     );


/*
 * Generate new keyblock backed by info.
 * Note that this theoretically "can't fail".
 */

typedef rpc_key_info_p_t (*rpc_dg_auth_new_key_fn_t) (
        rpc_auth_info_p_t               /*info*/
    );

typedef struct rpc_dg_auth_epv_t
{
    unsigned32 auth_proto;
    unsigned32 overhead;
    unsigned32 blocksize;
    rpc_dg_auth_create_fn_t create;
    rpc_dg_auth_pre_call_fn_t pre_call;
    rpc_dg_auth_encrypt_fn_t encrypt;
    rpc_dg_auth_pre_send_fn_t pre_send;
    rpc_dg_auth_recv_ck_fn_t recv_ck;
    rpc_dg_auth_way_fn_t way;
    rpc_dg_auth_way_handler_fn_t way_handler;
    rpc_dg_auth_new_key_fn_t new_key;
} rpc_dg_auth_epv_t, *rpc_dg_auth_epv_p_t;



/* =============================================================================== */

/*
 * Statistics of all forms
 */

typedef struct
{
    unsigned32 calls_sent;          /* # of remote calls made */
    unsigned32 calls_rcvd;          /* # of remote calls processed */
    unsigned32 pkts_sent;           /* Total # of pkts sent */
    unsigned32 pkts_rcvd;           /* Total # of pkts rcvd */
    unsigned32 brds_sent;           /* Total # of broadcast pkts sent */
    unsigned32 dups_sent;           /* # of duplicate frags sent */
    unsigned32 dups_rcvd;           /* # of duplicate frags rcvd */
    unsigned32 oo_rcvd;             /* # of out or order pkts rcvd */
    struct dg_pkt_stats_t              /* Breakdown of pkts sent/rcvd by pkt type */
    {                               
        unsigned32 sent;
        unsigned32 rcvd;
    } pstats[RPC_C_DG_PT_MAX_TYPE + 1];
} rpc_dg_stats_t;
#define RPC_DG_STATS_INITIALIZER {0, 0, 0, 0, 0, 0, 0, 0, {{0, 0}}}

#define RPC_DG_STATS_INCR(s) (rpc_g_dg_stats.s++)

PRIVATE void rpc__dg_stats_print ( void );

/* ========================================================================= */

/*
 * Receive dispatch flags.
 *
 * Dispatch flags are returned by the per-packet-type handler routines
 * called in the listener thread (from "recv_dispatch" in "dglsn.c").
 * These routines return certain information up to the common code that
 * handles all types of packets through a "rpc_dg_dflags_t" return value,
 * which is a bit set.  The various "rpc_c_dg_rdf_..." constants are
 * the possible members of the set.
 */

typedef unsigned32 rpc_dg_rd_flags_t;

#define RPC_C_DG_RDF_FREE_RQE  0x00000001   /* Free rqe--not enqueued by callee */
#define RPC_C_DG_RDF_YIELD     0x00000002   /* Yield before processing socket again */

/* =============================================================================== */
                           
/*
 * Prototype of the DG fork handling routine.
 */
void rpc__ncadg_fork_handler ( rpc_fork_stage_id_t /*stage*/);


/*
 * Prototypes of routines used in the DG RPC Protocol Service (call)
 */

PRIVATE rpc_call_rep_p_t rpc__dg_call_start (
        rpc_binding_rep_p_t  /*h*/,
        unsigned32  /*options*/,
        rpc_if_rep_p_t  /*ifspec*/, 
        unsigned32  /*opn*/, 
        rpc_transfer_syntax_t * /*transfer_syntax*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_transmit (
        rpc_call_rep_p_t  /*call*/, 
        rpc_iovector_p_t  /*data*/, 
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_transceive (
        rpc_call_rep_p_t  /*call*/, 
        rpc_iovector_p_t  /*send_data*/, 
        rpc_iovector_elt_t * /*recv_data*/, 
        ndr_format_t * /*ndr_format*/, 
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_receive (
        rpc_call_rep_p_t  /*call*/, 
        rpc_iovector_elt_t * /*data*/, 
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_end (
        rpc_call_rep_p_t * /*call*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_block_until_free (
        rpc_call_rep_p_t  /*call*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_alert (
        rpc_call_rep_p_t  /*call*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_fault (
        rpc_call_rep_p_t  /*call*/,
        rpc_iovector_p_t  /*fault_info*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_call_receive_fault (
        rpc_call_rep_p_t  /*call*/, 
        rpc_iovector_elt_t * /*data*/, 
        ndr_format_t * /*remote_ndr_format*/,
        unsigned32 * /*st*/
    );
PRIVATE boolean32 rpc__dg_call_did_mgr_execute (
        rpc_call_rep_p_t  /*call*/,
        unsigned32 * /*st*/
    );

/* =============================================================================== */

/*
 * Prototypes of routines used in the DG RPC Protocol Service (network).
 */

PRIVATE pointer_t rpc__dg_network_init_desc (
        rpc_socket_t * /*sock*/,
        rpc_protseq_id_t  /*pseq_id*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_use_protseq (
        rpc_protseq_id_t  /*pseq_id*/,
        unsigned32  /*max_calls*/,
        rpc_addr_p_t  /*rpc_addr*/,
        unsigned_char_p_t  /*endpoint*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_mon (
        rpc_binding_rep_p_t  /*binding_r*/,
        rpc_client_handle_t  /*client_h*/,
        rpc_network_rundown_fn_t  /*rundown*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_stop_mon (
        rpc_binding_rep_p_t  /*binding_r*/,
        rpc_client_handle_t  /*client_h*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_maint (
        rpc_binding_rep_p_t  /*binding_r*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_stop_maint (
        rpc_binding_rep_p_t  /*binding_r*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_close (
        rpc_binding_rep_p_t  /*binding_r*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_select_dispatch (
        rpc_socket_t  /*desc*/,
        pointer_t  /*si*/,
        boolean32  /*is_active*/,
        unsigned32 * /*st*/
    );
PRIVATE void rpc__dg_network_inq_prot_vers (
        unsigned8 * /*prot_id*/,
        unsigned32 * /*vers_major*/,
        unsigned32 * /*vers_minor*/,
        unsigned32 * /*st*/
    );

/* =============================================================================== */

/*
 * Prototypes of routines used in the DG RPC Protocol Service (mgmt)
 */

PRIVATE unsigned32 rpc__dg_mgmt_inq_calls_sent ( void );
PRIVATE unsigned32 rpc__dg_mgmt_inq_calls_rcvd ( void );
PRIVATE unsigned32 rpc__dg_mgmt_inq_pkts_sent ( void );
PRIVATE unsigned32 rpc__dg_mgmt_inq_pkts_rcvd ( void );

/* ========================================================================= */

PRIVATE void rpc__dg_monitor_init (void);

PRIVATE void rpc__dg_monitor_fork_handler (
        rpc_fork_stage_id_t /*stage*/
    );

PRIVATE void rpc__dg_maintain_init (void);

PRIVATE void rpc__dg_maintain_fork_handler (
        rpc_fork_stage_id_t /*stage*/
    );

PRIVATE void rpc__dg_call_transmit_int (
        rpc_dg_call_p_t  /*call*/,
        rpc_iovector_p_t  /*data*/, 
        unsigned32 * /*st*/
    );

PRIVATE void rpc__dg_call_receive_int (
        rpc_dg_call_p_t  /*call*/,
        rpc_iovector_elt_t * /*data*/, 
        unsigned32 * /*st*/
    );

PRIVATE boolean32 rpc__dg_handle_conv (
        rpc_socket_t  /*sock*/,
        rpc_dg_recvq_elt_p_t /*rqe*/
    );

PRIVATE void rpc__dg_convc_indy ( dce_uuid_p_t /*cas_uuid*/);


PRIVATE void rpc__dg_handle_convc ( 
        rpc_dg_recvq_elt_p_t /*rqe*/
    );

#ifdef PASSWD_ETC
PRIVATE void rpc__dg_handle_rgy (
        rpc_socket_t  /*sock*/,
        rpc_dg_recvq_elt_p_t /*rqe*/
    );
#endif

PRIVATE void rpc__dg_get_epkt_body_st (
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        unsigned32 * /*st*/
    );

/* ========================================================================= */

#include <dgsoc.h>
#include <dgglob.h>
#include <dgutl.h>

/* ========================================================================= */

PRIVATE void rpc__dg_conv_init ( void );

PRIVATE void rpc__dg_conv_fork_handler ( 
    rpc_fork_stage_id_t /*stage*/
    );

PRIVATE void rpc__dg_fack_common    (
        rpc_dg_sock_pool_elt_p_t  /*sp*/,
        rpc_dg_recvq_elt_p_t  /*rqe*/,
        rpc_dg_call_p_t  /*call*/,
        boolean * /*sent_data*/
    );

/* ======================================================================== */

#ifdef __cplusplus
}
#endif


#endif /* _DG_H */

