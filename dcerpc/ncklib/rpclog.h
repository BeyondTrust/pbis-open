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
#ifndef _RPCLOG_H
#define _RPCLOG_H
/*
**
**  NAME
**
**      rpclog.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Interface to performance logging service.
**
**
*/

#ifdef LOGPTS
#define MODULE_MASK             0xff00
#define EVENT_MASK              0x00ff
#define RPC_LOG_INITIALIZE      { if (rpc__log_ptr_init()) {rpc_g_log_ptr = rpc__log_ptr_init();}}
#define LOG_RPC(code)           TIMESTAMP (0x0300 | (EVENT_MASK & code))
#define TIMESTAMP(code)         {*rpc_g_log_ptr = (code);}

/*
 * address of log point for drq3b on Q bus
 */

#define LOGPT_IN_LOGPT_CSRS     8
#define LOGPT_CSRS              0160740
#define LOGPT_IN_IO_PAGE        ((LOGPT_CSRS  & 017777) + LOGPT_IN_LOGPT_CSRS)
#define IO_PAGE_IN_QMEM         (1<<22)
#define LOGPT_ADDR_IN_QMEM      LOGPT_IN_IO_PAGE + IO_PAGE_IN_QMEM

typedef unsigned short rpc_logpt_t;
typedef rpc_logpt_t  *rpc_logpt_ptr_t;
extern  rpc_logpt_ptr_t rpc__log_ptr_init ();
extern  rpc_logpt_ptr_t rpc_g_log_ptr;
#else
#define RPC_LOG_INITIALIZE
#define LOG_RPC(code)
#endif /* LOGPTS */


#define	RPC_LOG_CLIENT_STUB_NTR         LOG_RPC (0x00)
#define	RPC_LOG_CLIENT_STUB_XIT         LOG_RPC (0x01)
#define	RPC_LOG_SERVER_STUB_PRE         LOG_RPC (0x02)
#define	RPC_LOG_SERVER_STUB_POST        LOG_RPC (0x03)

#define	RPC_LOG_CALL_START_NTR          LOG_RPC (0x04)
#define	RPC_LOG_CALL_START_XIT          LOG_RPC (0x05)
#define	RPC_LOG_CALL_TRANSMIT_NTR       LOG_RPC (0x06)
#define	RPC_LOG_CALL_TRANSMIT_XIT       LOG_RPC (0x07)
#define	RPC_LOG_CALL_TRANSCEIVE_NTR     LOG_RPC (0x08)
#define	RPC_LOG_CALL_TRANSCEIVE_XIT     LOG_RPC (0x09)
#define	RPC_LOG_CALL_RECEIVE_NTR        LOG_RPC (0x0a)
#define	RPC_LOG_CALL_RECEIVE_XIT        LOG_RPC (0x0b)
#define	RPC_LOG_CALL_END_NTR            LOG_RPC (0x0c)
#define	RPC_LOG_CALL_END_XIT            LOG_RPC (0x0d)

#define RPC_LOG_CN_CALL_START_NTR       LOG_RPC (0x0e)
#define RPC_LOG_CN_CALL_START_XIT       LOG_RPC (0x0f)
#define RPC_LOG_CN_CALL_TRANSMIT_NTR    LOG_RPC (0x10)
#define RPC_LOG_CN_CALL_TRANSMIT_XIT    LOG_RPC (0x11)
#define RPC_LOG_CN_CALL_TRANSCEIVE_NTR  LOG_RPC (0x12)
#define RPC_LOG_CN_CALL_TRANSCEIVE_XIT  LOG_RPC (0x13)
#define RPC_LOG_CN_CALL_RECEIVE_NTR     LOG_RPC (0x14)
#define RPC_LOG_CN_CALL_RECEIVE_XIT     LOG_RPC (0x15)
#define RPC_LOG_CN_CALL_END_NTR         LOG_RPC (0x16)
#define RPC_LOG_CN_CALL_END_XIT         LOG_RPC (0x17)

#define RPC_LOG_CN_CTHD_NTR             LOG_RPC (0x18)
#define RPC_LOG_CN_CTHD_XIT             LOG_RPC (0x19)

#define RPC_LOG_CN_ASSOC_REQ_NTR        LOG_RPC (0x1a)
#define RPC_LOG_CN_ASSOC_REQ_XIT        LOG_RPC (0x1b)
#define RPC_LOG_CN_ASSOC_LIS_NTR        LOG_RPC (0x1c)
#define RPC_LOG_CN_ASSOC_LIS_XIT        LOG_RPC (0x1d)
#define RPC_LOG_CN_ASSOC_ALLOC_NTR      LOG_RPC (0x1e)
#define RPC_LOG_CN_ASSOC_ALLOC_XIT      LOG_RPC (0x1f)
#define RPC_LOG_CN_ASSOC_DEALLOC_NTR    LOG_RPC (0x20)
#define RPC_LOG_CN_ASSOC_DEALLOC_XIT    LOG_RPC (0x21)
#define RPC_LOG_CN_ASSOC_POP_CALL_NTR   LOG_RPC (0x22)
#define RPC_LOG_CN_ASSOC_POP_CALL_XIT   LOG_RPC (0x23)
#define RPC_LOG_CN_ASSOC_PUSH_CALL_NTR  LOG_RPC (0x24)
#define RPC_LOG_CN_ASSOC_PUSH_CALL_XIT  LOG_RPC (0x25)
#define RPC_LOG_CN_ASSOC_Q_FRAG_NTR     LOG_RPC (0x26)
#define RPC_LOG_CN_ASSOC_Q_FRAG_XIT     LOG_RPC (0x27)
#define RPC_LOG_CN_ASSOC_RECV_FRAG_NTR  LOG_RPC (0x28)
#define RPC_LOG_CN_ASSOC_RECV_FRAG_XIT  LOG_RPC (0x29)
#define RPC_LOG_CN_ASSOC_SEND_FRAG_NTR  LOG_RPC (0x2a)
#define RPC_LOG_CN_ASSOC_SEND_FRAG_XIT  LOG_RPC (0x2b)
#define RPC_LOG_CN_ASSOC_SYN_NEG_NTR    LOG_RPC (0x2c)
#define RPC_LOG_CN_ASSOC_SYN_NEG_XIT    LOG_RPC (0x2d)
#define RPC_LOG_CN_ASSOC_SYN_LKUP_NTR   LOG_RPC (0x2e)
#define RPC_LOG_CN_ASSOC_SYN_LKUP_XIT   LOG_RPC (0x2f)
#define RPC_LOG_CN_ASSOC_ACB_CR_NTR     LOG_RPC (0x30)
#define RPC_LOG_CN_ASSOC_ACB_CR_XIT     LOG_RPC (0x31)
#define RPC_LOG_CN_ASSOC_ACB_FR_NTR     LOG_RPC (0x32)
#define RPC_LOG_CN_ASSOC_ACB_FR_XIT     LOG_RPC (0x33)
#define RPC_LOG_CN_ASSOC_ACB_DEAL_NTR   LOG_RPC (0x34)
#define RPC_LOG_CN_ASSOC_ACB_DEAL_XIT   LOG_RPC (0x35)

#define RPC_LOG_CN_GRP_ADDR_LKUP_NTR    LOG_RPC (0x36)
#define RPC_LOG_CN_GRP_ADDR_LKUP_XIT    LOG_RPC (0x37)
#define RPC_LOG_CN_GRP_ID_LKUP_NTR      LOG_RPC (0x38)
#define RPC_LOG_CN_GRP_ID_LKUP_XIT      LOG_RPC (0x39)
#define RPC_LOG_03A                     LOG_RPC (0x3a)
#define RPC_LOG_03B                     LOG_RPC (0x3b)
#define RPC_LOG_03C                     LOG_RPC (0x3c)
#define RPC_LOG_03D                     LOG_RPC (0x3d)
#define RPC_LOG_03E                     LOG_RPC (0x3e)
#define RPC_LOG_03F                     LOG_RPC (0x3f)
#define RPC_LOG_CN_GRP_REMID_LKUP_NTR   LOG_RPC (0x40)
#define RPC_LOG_CN_GRP_REMID_LKUP_XIT   LOG_RPC (0x41)

#define RPC_LOG_SOCKET_OPEN_NTR         LOG_RPC (0x42)
#define RPC_LOG_SOCKET_OPEN_XIT         LOG_RPC (0x43)
#define RPC_LOG_SOCKET_CLOSE_NTR        LOG_RPC (0x44)
#define RPC_LOG_SOCKET_CLOSE_XIT        LOG_RPC (0x45)
#define RPC_LOG_SOCKET_BIND_NTR         LOG_RPC (0x46)
#define RPC_LOG_SOCKET_BIND_XIT         LOG_RPC (0x47)
#define RPC_LOG_SOCKET_CONNECT_NTR      LOG_RPC (0x48)
#define RPC_LOG_SOCKET_CONNECT_XIT      LOG_RPC (0x49)
#define RPC_LOG_SOCKET_ACCEPT_NTR       LOG_RPC (0x4a)
#define RPC_LOG_SOCKET_ACCEPT_XIT       LOG_RPC (0x4b)
#define RPC_LOG_SOCKET_LISTEN_NTR       LOG_RPC (0x4c)
#define RPC_LOG_SOCKET_LISTEN_XIT       LOG_RPC (0x4d)
#define RPC_LOG_SOCKET_SENDMSG_NTR      LOG_RPC (0x4e)
#define RPC_LOG_SOCKET_SENDMSG_XIT      LOG_RPC (0x4f)
#define RPC_LOG_SOCKET_RECVFROM_NTR     LOG_RPC (0x50)
#define RPC_LOG_SOCKET_RECVFROM_XIT     LOG_RPC (0x51)
#define RPC_LOG_SOCKET_RECVMSG_NTR      LOG_RPC (0x52)
#define RPC_LOG_SOCKET_RECVMSG_XIT      LOG_RPC (0x53)
#define RPC_LOG_SOCKET_INQ_EP_NTR       LOG_RPC (0x54)
#define RPC_LOG_SOCKET_INQ_EP_XIT       LOG_RPC (0x55)

#define	RPC_LOG_SELECT_PRE              LOG_RPC (0x56)
#define	RPC_LOG_SELECT_POST             LOG_RPC (0x57)

#define RPC_LOG_MUTEX_INIT_NTR          LOG_RPC (0x58)
#define RPC_LOG_MUTEX_INIT_XIT          LOG_RPC (0x59)
#define RPC_LOG_MUTEX_DELETE_NTR        LOG_RPC (0x5a)
#define RPC_LOG_MUTEX_DELETE_XIT        LOG_RPC (0x5b)
#define RPC_LOG_MUTEX_LOCK_NTR          LOG_RPC (0x5c)
#define RPC_LOG_MUTEX_LOCK_XIT          LOG_RPC (0x5d)
#define RPC_LOG_MUTEX_TRY_LOCK_NTR      LOG_RPC (0x5e)
#define RPC_LOG_MUTEX_TRY_LOCK_XIT      LOG_RPC (0x5f)
#define RPC_LOG_MUTEX_UNLOCK_NTR        LOG_RPC (0x60)
#define RPC_LOG_MUTEX_UNLOCK_XIT        LOG_RPC (0x61)
#define RPC_LOG_MUTEX_LOCK_ASRT_NTR     LOG_RPC (0x62)
#define RPC_LOG_MUTEX_LOCK_ASRT_XIT     LOG_RPC (0x63)
#define RPC_LOG_MUTEX_UNLOCK_ASRT_NTR   LOG_RPC (0x64)
#define RPC_LOG_MUTEX_UNLOCK_ASRT_XIT   LOG_RPC (0x65)
#define RPC_LOG_COND_INIT_NTR           LOG_RPC (0x66)
#define RPC_LOG_COND_INIT_XIT           LOG_RPC (0x67)
#define RPC_LOG_COND_DELETE_NTR         LOG_RPC (0x68)
#define RPC_LOG_COND_DELETE_XIT         LOG_RPC (0x69)
#define RPC_LOG_COND_WAIT_NTR           LOG_RPC (0x6a)
#define RPC_LOG_COND_WAIT_XIT           LOG_RPC (0x6b)
#define RPC_LOG_COND_TIMED_WAIT_NTR     LOG_RPC (0x6c)
#define RPC_LOG_COND_TIMED_WAIT_XIT     LOG_RPC (0x6d)
#define RPC_LOG_COND_SIGNAL_NTR         LOG_RPC (0x6e)
#define RPC_LOG_COND_SIGNAL_XIT         LOG_RPC (0x6f)
#define RPC_LOG_COND_BROADCAST_NTR      LOG_RPC (0x70)
#define RPC_LOG_COND_BROADCAST_XIT      LOG_RPC (0x71)

#define RPC_LOG_MEM_ALLOC_NTR           LOG_RPC (0x72)
#define RPC_LOG_MEM_ALLOC_XIT           LOG_RPC (0x73)
#define RPC_LOG_MEM_REALLOC_NTR         LOG_RPC (0x74)
#define RPC_LOG_MEM_REALLOC_XIT         LOG_RPC (0x75)
#define RPC_LOG_MEM_FREE_NTR            LOG_RPC (0x76)
#define RPC_LOG_MEM_FREE_XIT            LOG_RPC (0x77)

#define RPC_LOG_LIST_ELT_ALLOC_NTR      LOG_RPC (0x78)
#define RPC_LOG_LIST_ELT_ALLOC_XIT      LOG_RPC (0x79)
#define RPC_LOG_LIST_ELT_FREE_NTR       LOG_RPC (0x7a)
#define RPC_LOG_LIST_ELT_FREE_XIT       LOG_RPC (0x7b)

#define RPC_LOG_IF_LOOKUP_NTR           LOG_RPC (0x7c)
#define RPC_LOG_IF_LOOKUP_XIT           LOG_RPC (0x7d)
#define RPC_LOG_07E                     LOG_RPC (0x7e)
#define RPC_LOG_07F                     LOG_RPC (0x7f)

#define RPC_LOG_NAF_ADDR_ALLOC_NTR      LOG_RPC (0x80)
#define RPC_LOG_NAF_ADDR_ALLOC_XIT      LOG_RPC (0x81)
#define RPC_LOG_NAF_ADDR_COPY_NTR       LOG_RPC (0x82)
#define RPC_LOG_NAF_ADDR_COPY_XIT       LOG_RPC (0x83)
#define RPC_LOG_NAF_ADDR_FREE_NTR       LOG_RPC (0x84)
#define RPC_LOG_NAF_ADDR_FREE_XIT       LOG_RPC (0x85)

#define RPC_LOG_UUID_EQUAL_NTR          LOG_RPC (0x86)
#define RPC_LOG_UUID_EQUAL_XIT          LOG_RPC (0x87)
#define RPC_LOG_088                     LOG_RPC (0x88)
#define RPC_LOG_089                     LOG_RPC (0x89)
#define RPC_LOG_UUID_HASH_NTR           LOG_RPC (0x8a)
#define RPC_LOG_UUID_HASH_XIT           LOG_RPC (0x8b)
#define RPC_LOG_UUID_CREATE_NTR         LOG_RPC (0x8c)
#define RPC_LOG_UUID_CREATE_XIT         LOG_RPC (0x8d)

#define RPC_LOG_CN_RCV_PKT_NTR          LOG_RPC (0x8e)
#define RPC_LOG_CN_RCV_PKT_XIT          LOG_RPC (0x8f)
#define RPC_LOG_090                     LOG_RPC (0x90)
#define RPC_LOG_091                     LOG_RPC (0x91)
#define RPC_LOG_CN_PROCESS_PKT_NTR      LOG_RPC (0x92)
#define RPC_LOG_CN_PROCESS_PKT_XIT      LOG_RPC (0x93)

#define RPC_LOG_SERVER_COND_WAIT_PRE    LOG_RPC (0x94)
#define RPC_LOG_SERVER_COND_WAIT_POST   LOG_RPC (0x95)
#define RPC_LOG_SERVER_RECVMSG_PRE      LOG_RPC (0x96)
#define RPC_LOG_SERVER_RECVMSG_POST     LOG_RPC (0x97)
#define RPC_LOG_SERVER_SENDMSG_PRE      LOG_RPC (0x98)
#define RPC_LOG_SERVER_SENDMSG_POST     LOG_RPC (0x99)
#define RPC_LOG_SERVER_YIELD_PRE        LOG_RPC (0x9a)
#define RPC_LOG_SERVER_YIELD_POST       LOG_RPC (0x9b)
#define RPC_LOG_SERVER_COND_SIG_PRE     LOG_RPC (0x9c)
#define RPC_LOG_SERVER_COND_SIG_POST    LOG_RPC (0x9d)

#define RPC_LOG_CLIENT_COND_WAIT_PRE    LOG_RPC (0x9e)
#define RPC_LOG_CLIENT_COND_WAIT_POST   LOG_RPC (0x9f)
#define RPC_LOG_CLIENT_RECVMSG_PRE      LOG_RPC (0xa0)
#define RPC_LOG_CLIENT_RECVMSG_POST     LOG_RPC (0xa1)
#define RPC_LOG_CLIENT_SENDMSG_PRE      LOG_RPC (0xa2)
#define RPC_LOG_CLIENT_SENDMSG_POST     LOG_RPC (0xa3)
#define RPC_LOG_CLIENT_YIELD_PRE        LOG_RPC (0xa4)
#define RPC_LOG_CLIENT_YIELD_POST       LOG_RPC (0xa5)
#define RPC_LOG_CLIENT_COND_SIG_PRE     LOG_RPC (0xa6)
#define RPC_LOG_CLIENT_COND_SIG_POST    LOG_RPC (0xa7)

#define RPC_LOG_SERVER_LOOP_NTR         LOG_RPC (0xa8)
#define RPC_LOG_SERVER_LOOP_XIT         LOG_RPC (0xa9)
#define RPC_LOG_CLIENT_LOOP_NTR         LOG_RPC (0xaa)
#define RPC_LOG_CLIENT_LOOP_XIT         LOG_RPC (0xab)

#define RPC_LOG_TRY_PRE                 LOG_RPC (0xac)
#define RPC_LOG_TRY_POST                LOG_RPC (0xad)
#define RPC_LOG_CATCH_PRE               LOG_RPC (0xae)
#define RPC_LOG_CATCH_POST              LOG_RPC (0xaf)
#define RPC_LOG_FINALLY_PRE             LOG_RPC (0xb0)
#define RPC_LOG_FINALLY_POST            LOG_RPC (0xb1)
#define RPC_LOG_ENDTRY_PRE              LOG_RPC (0xb2)
#define RPC_LOG_ENDTRY_POST             LOG_RPC (0xb3)

#endif /* _RPCLOG_H */
