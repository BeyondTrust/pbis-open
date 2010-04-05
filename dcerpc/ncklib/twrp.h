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
#ifndef _TWRP_H
#define _TWRP_H	1
/*
**
**  NAME
**
**      twrp.h
**
**  FACILITY:
**
**      Protocol Tower Services
**
**  ABSTRACT:
**
**      Private protocol tower service typedefs, constant definitions, etc.
**
**
*/

/*
 * Protocol identifiers for each lower tower floor.
 */
#define TWR_C_FLR_PROT_ID_DNA           0x02 /* DNA     */
#define TWR_C_FLR_PROT_ID_OSI           0x03 /* OSI     */
#define TWR_C_FLR_PROT_ID_NSP           0x04 /* NSP     */
#define TWR_C_FLR_PROT_ID_TP4           0x05 /* TP4     */
#define TWR_C_FLR_PROT_ID_ROUTING       0x06 /* Routing */
#define TWR_C_FLR_PROT_ID_TCP           0x07 /* TCP     */
#define TWR_C_FLR_PROT_ID_UDP           0x08 /* UDP     */
#define TWR_C_FLR_PROT_ID_IP            0x09 /* IP      */
#define TWR_C_FLR_PROT_ID_NP            0x0F /* SMB Named Pipes */
#define TWR_C_FLR_PROT_ID_NT_NP         0x10 /* NT Named Pipes */
#define TWR_C_FLR_PROT_ID_NB            0x11 /* NetBIOS */
#define TWR_C_FLR_PROT_ID_NETBEUI       0x12 /* NetBEUI */
#define TWR_C_FLR_PROT_ID_SPX           0x13 /* NetWare SPX */
#define TWR_C_FLR_PROT_ID_IPX           0x14 /* NetWare IPX */
#define TWR_C_FLR_PROT_ID_VNS_SPP       0x1A /* VINES SPP */
#define TWR_C_FLR_PROT_ID_VNS_IPC       0x1B /* VINES IPC */
#define TWR_C_FLR_PROT_ID_UXD           0x20 /* Unix-Domain Socket */
#define TWR_C_FLR_PROT_ID_NMB           0x22 /* NetBIOS name */

/*
 * Number of lower floors in each address family.
 */
#define TWR_C_NUM_UXD_LOWER_FLRS      1  /* Number lower flrs in uxd tower  */
#define TWR_C_NUM_IP_LOWER_FLRS       2  /* Number lower flrs in ip tower  */
#define TWR_C_NUM_DNA_LOWER_FLRS      3  /* Number lower flrs in dna tower */
#define TWR_C_NUM_OSI_LOWER_FLRS      3  /* Number lower flrs in osi tower */
#define TWR_C_NUM_DDS_LOWER_FLRS      2  /* Number lower flrs in dds tower  */
#define TWR_C_NUM_NP_LOWER_FLRS       2  /* Number lower flrs in np tower */

/*
 * Number of bytes overhead per floor = protocol identifier (lhs) count 
 * unsigned (2) + protocol identifier (1) + address data (rhs) count unsigned (2)
 */
#define TWR_C_FLR_OVERHEAD  5  

/*
 * If (and when) the twr facility is separated from rpc, modify 
 * these twr_c_* to replace the rpc_c_* with the underlying constant.
 */

/*
 * Number of bytes in the tower floor count field 
 */

#define  TWR_C_TOWER_FLR_COUNT_SIZE  RPC_C_TOWER_FLR_COUNT_SIZE

/*
 * Number of bytes in the lhs count field of a floor.
 */
#define   TWR_C_TOWER_FLR_LHS_COUNT_SIZE  RPC_C_TOWER_FLR_LHS_COUNT_SIZE

/*
 * Number of bytes in the rhs count field of a floor.
 */
#define   TWR_C_TOWER_FLR_RHS_COUNT_SIZE  RPC_C_TOWER_FLR_RHS_COUNT_SIZE

/*
 * Number of bytes for storing a major or minor version.
 */
#define  TWR_C_TOWER_VERSION_SIZE  RPC_C_TOWER_VERSION_SIZE  

/*
 * Number of bytes for storing a tower floor protocol id or protocol id prefix.
 */

#define  TWR_C_TOWER_PROT_ID_SIZE  RPC_C_TOWER_PROT_ID_SIZE

/*
 * Number of bytes for storing a uuid.
 */
#define  TWR_C_TOWER_UUID_SIZE  RPC_C_TOWER_UUID_SIZE  

/*
 * IP family port and address sizes
 */
#define  TWR_C_IP_PORT_SIZE  2
#define  TWR_C_IP_ADDR_SIZE  4

#endif /* _TWRP_H */
