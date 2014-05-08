/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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

#ifndef __SMBWIRE_H__
#define __SMBWIRE_H__

#define WIRE_NTTIME_EPOCH_DIFFERENCE_SECS             (11644473600LL)

#define WIRE_FACTOR_MICROSECS_TO_HUNDREDS_OF_NANOSECS (10LL)

#define WIRE_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS \
                        (1000LL * WIRE_FACTOR_MICROSECS_TO_HUNDREDS_OF_NANOSECS)

#define WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS \
                        (1000LL * WIRE_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS)

typedef enum
{
    /*
     * Using an enum permits values to be inspected more easily in gdb, but
     * note that an enum will likely be larger than the on-wire representation
     * (four bytes vs. one byte).  There is no standard way to produce a
     * short enum.
    */
    /* Most of these are obsolete (pre-Win2k) */
    COM_CREATE_DIRECTORY        = 0x00,
    COM_DELETE_DIRECTORY        = 0x01,
    COM_OPEN                    = 0x02,
    COM_CREATE                  = 0x03,
    COM_CLOSE                   = 0x04,
    COM_FLUSH                   = 0x05,
    COM_DELETE                  = 0x06,
    COM_RENAME                  = 0x07,
    COM_QUERY_INFORMATION       = 0x08,
    COM_SET_INFORMATION         = 0x09,
    COM_READ                    = 0x0A,
    COM_WRITE                   = 0x0B,
    COM_LOCK_BYTE_RANGE         = 0x0C,
    COM_UNLOCK_BYTE_RANGE       = 0x0D,
    COM_CREATE_TEMPORARY        = 0x0E,
    COM_CREATE_NEW              = 0x0F,
    COM_CHECK_DIRECTORY         = 0x10,
    COM_PROCESS_EXIT            = 0x11,
    COM_SEEK                    = 0x12,
    COM_LOCK_AND_READ           = 0x13,
    COM_WRITE_AND_UNLOCK        = 0x14,
    COM_READ_RAW                = 0x1A,
    COM_READ_MPX                = 0x1B,
    COM_READ_MPX_SECONDARY      = 0x1C,
    COM_WRITE_RAW               = 0x1D,
    COM_WRITE_MPX               = 0x1E,
    COM_WRITE_MPX_SECONDARY     = 0x1F,
    COM_WRITE_COMPLETE          = 0x20,
    COM_QUERY_SERVER            = 0x21,
    COM_SET_INFORMATION2        = 0x22,
    COM_QUERY_INFORMATION2      = 0x23,
    COM_LOCKING_ANDX            = 0x24,
    COM_TRANSACTION             = 0x25,
    COM_TRANSACTION_SECONDARY   = 0x26,
    COM_IOCTL                   = 0x27,
    COM_IOCTL_SECONDARY         = 0x28,
    COM_COPY                    = 0x29,
    COM_MOVE                    = 0x2A,
    COM_ECHO                    = 0x2B,
    COM_WRITE_AND_CLOSE         = 0x2C,
    COM_OPEN_ANDX               = 0x2D,
    COM_READ_ANDX               = 0x2E,
    COM_WRITE_ANDX              = 0x2F,
    COM_NEW_FILE_SIZE           = 0x30,
    COM_CLOSE_AND_TREE_DISC     = 0x31,
    COM_TRANSACTION2            = 0x32,
    COM_TRANSACTION2_SECONDARY  = 0x33,
    COM_FIND_CLOSE2             = 0x34,
    COM_FIND_NOTIFY_CLOSE       = 0x35,
    /* Used by Xenix/Unix 0x60 - 0x6E */
    COM_TREE_CONNECT            = 0x70,
    COM_TREE_DISCONNECT         = 0x71,
    COM_NEGOTIATE               = 0x72,
    COM_SESSION_SETUP_ANDX      = 0x73,
    COM_LOGOFF_ANDX             = 0x74,
    COM_TREE_CONNECT_ANDX       = 0x75,
    COM_QUERY_INFORMATION_DISK  = 0x80,
    COM_SEARCH                  = 0x81,
    COM_FIND                    = 0x82,
    COM_FIND_UNIQUE             = 0x83,
    COM_FIND_CLOSE              = 0x84,
    COM_NT_TRANSACT             = 0xA0,
    COM_NT_TRANSACT_SECONDARY   = 0xA1,
    COM_NT_CREATE_ANDX          = 0xA2,
    COM_NT_CANCEL               = 0xA4,
    COM_NT_RENAME               = 0xA5,
    COM_OPEN_PRINT_FILE         = 0xC0,
    COM_WRITE_PRINT_FILE        = 0xC1,
    COM_CLOSE_PRINT_FILE        = 0xC2,
    COM_GET_PRINT_QUEUE         = 0xC3,
    COM_READ_BULK               = 0xD8,
    COM_WRITE_BULK              = 0xD9,
    COM_WRITE_BULK_DATA         = 0xDA,
    COM_LW_OPLOCK               = 0xDD,
    COM_NO_ANDX_COMMAND         = 0xFF
} COMMAND;

typedef USHORT SMB_FILE_ATTRIBUTES, *PSMB_FILE_ATTRIBUTES;

// used as inclusive search attributes
#define SMB_FILE_ATTRIBUTE_NORMAL       0x0000
#define SMB_FILE_ATTRIBUTE_READONLY     0x0001
#define SMB_FILE_ATTRIBUTE_HIDDEN       0x0002
#define SMB_FILE_ATTRIBUTE_SYSTEM       0x0004
#define SMB_FILE_ATTRIBUTE_VOLUME       0x0008
#define SMB_FILE_ATTRIBUTE_DIRECTORY    0x0010
#define SMB_FILE_ATTRIBUTE_ARCHIVE      0x0020

// exclusive search attributes
#define SMB_SEARCH_ATTRIBUTE_READONLY   0x0100
#define SMB_SEARCH_ATTRIBUTE_HIDDEN     0x0200
#define SMB_SEARCH_ATTRIBUTE_SYSTEM     0x0400
#define SMB_SEARCH_ATTRIBUTE_VOLUME     0x0800
#define SMB_SEARCH_ATTRIBUTE_DIRECTORY  0x1000
#define SMB_SEARCH_ATTRIBUTE_ARCHIVE    0x2000

#define SMB_FILE_ATTRIBUTES_MASK 0x3F

#define SMB_FILE_ATTRIBUTES_FROM_NATIVE(FileAttributes) \
    ((SMB_FILE_ATTRIBUTES)((FileAttributes) & SMB_FILE_ATTRIBUTES_MASK))

#define SMB_FILE_ATTRIBUTES_TO_NATIVE(SmbFileAttributes) \
    ((FILE_ATTRIBUTES)((SmbFileAttributes) & SMB_FILE_ATTRIBUTES_MASK))

#define SMB_SEARCH_ATTRIBUTES_TO_NATIVE(SearchAttributes) \
    SMB_FILE_ATTRIBUTES_TO_NATIVE(SearchAttributes >> 8)

typedef enum
{
	COM2_NEGOTIATE       = 0x00,
	COM2_SESSION_SETUP   = 0x01,
	COM2_LOGOFF          = 0x02,
	COM2_TREE_CONNECT    = 0x03,
	COM2_TREE_DISCONNECT = 0x04,
	COM2_CREATE          = 0x05,
	COM2_CLOSE           = 0x06,
	COM2_FLUSH           = 0x07,
	COM2_READ            = 0x08,
	COM2_WRITE           = 0x09,
	COM2_LOCK            = 0x0A,
	COM2_IOCTL           = 0x0B,
	COM2_CANCEL          = 0x0C,
	COM2_ECHO            = 0x0D,
	COM2_FIND            = 0x0E,
	COM2_NOTIFY          = 0x0F,
	COM2_GETINFO         = 0x10,
	COM2_SETINFO         = 0x11,
	COM2_BREAK           = 0x12
} SMB2_COMMAND;

/// These are dialect tags for SMBV1 negotiate
#define SMB_NEGOTIATE_RESPONSE_DIALECT_V2_0   0x0202
#define SMB_NEGOTIATE_RESPONSE_DIALECT_V2_1   0x02FF

/// These are dialect tags for SMBV2 negotiate
#define SMB2_NEGOTIATE_DIALECT_V2             0x0202
#define SMB2_NEGOTIATE_DIALECT_V2_1           0x0210


#define SMB2_NEGOTIATE_SECURITY_FLAG_SIGNING_ENABLED    0x01
#define SMB2_NEGOTIATE_SECURITY_FLAG_SIGNING_REQUIRED   0x02

#define SMB2_NEGOTIATE_CAPABILITY_FLAG_DFS              0x00000001
#define SMB2_NEGOTIATE_CAPABILITY_FLAG_LEASES           0x00000002
#define SMB2_NEGOTIATE_CAPABILITY_FLAG_LARGE_MTU        0x00000004


typedef ULONG SMB2_FLAGS;

#define SMB2_FLAGS_SERVER_TO_REDIR   0x00000001
#define SMB2_FLAGS_ASYNC_COMMAND     0x00000002
#define SMB2_FLAGS_RELATED_OPERATION 0x00000004
#define SMB2_FLAGS_SIGNED            0x00000008
#define SMB2_FLAGS_DFS_OPERATIONS    0x10000000

typedef USHORT SMB2_SESSION_FLAGS;

#define SMB2_SESSION_FLAGS_IS_GUEST_USER 0x0001
#define SMB2_SESSION_FLAGS_IS_NULL_USER  0x0002

typedef UCHAR SMB2_INFO_TYPE;

#define SMB2_INFO_TYPE_FILE        0x01
#define SMB2_INFO_TYPE_FILE_SYSTEM 0x02
#define SMB2_INFO_TYPE_SECURITY    0x03
#define SMB2_INFO_TYPE_QUOTA       0x04

typedef UCHAR SMB2_FILE_INFO_CLASS;

#define SMB2_FILE_INFO_CLASS_DIR             0x01
#define SMB2_FILE_INFO_CLASS_FULL_DIR        0x02
#define SMB2_FILE_INFO_CLASS_BOTH_DIR        0x03
#define SMB2_FILE_INFO_CLASS_BASIC           0x04
#define SMB2_FILE_INFO_CLASS_STANDARD        0x05
#define SMB2_FILE_INFO_CLASS_INTERNAL        0x06
#define SMB2_FILE_INFO_CLASS_EA              0x07
#define SMB2_FILE_INFO_CLASS_ACCESS          0x08
#define SMB2_FILE_INFO_CLASS_NAME            0x09
#define SMB2_FILE_INFO_CLASS_RENAME          0x0A
#define SMB2_FILE_INFO_CLASS_NAMES           0x0C
#define SMB2_FILE_INFO_CLASS_DISPOSITION     0x0D
#define SMB2_FILE_INFO_CLASS_POSITION        0x0E
#define SMB2_FILE_INFO_FULL_EA               0x0F
#define SMB2_FILE_INFO_CLASS_MODE            0x10
#define SMB2_FILE_INFO_CLASS_ALIGNMENT       0x11
#define SMB2_FILE_INFO_CLASS_ALL             0x12
#define SMB2_FILE_INFO_CLASS_ALLOCATION      0x13
#define SMB2_FILE_INFO_CLASS_EOF             0x14
#define SMB2_FILE_INFO_CLASS_ALTERNATE_NAME  0x15
#define SMB2_FILE_INFO_CLASS_STREAM          0x16
#define SMB2_FILE_INFO_CLASS_PIPE            0x17
#define SMB2_FILE_INFO_CLASS_PIPE_LOCAL      0x18
#define SMB2_FILE_INFO_CLASS_PIPE_REMOTE     0x19
#define SMB2_FILE_INFO_CLASS_MAILSLOT_QUERY  0x1A
#define SMB2_FILE_INFO_CLASS_COMPRESSION     0x1C
#define SMB2_FILE_INFO_CLASS_COMPLETION      0x1D
#define SMB2_FILE_INFO_CLASS_QUOTA           0x20
#define SMB2_FILE_INFO_CLASS_REPARSE_POINT   0x21
#define SMB2_FILE_INFO_CLASS_NETWORK_OPEN    0x22
#define SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG   0x23
#define SMB2_FILE_INFO_CLASS_ID_BOTH_DIR     0x25
#define SMB2_FILE_INFO_CLASS_ID_FULL_DIR     0x26
#define SMB2_FILE_INFO_CLASS_FS_ATTRIBUTE    0x29
#define SMB2_FILE_INFO_CLASS_FS_SIZE         0x2B
#define SMB2_FILE_INFO_CLASS_SFIO_RESERVE    0x2C
#define SMB2_FILE_INFO_CLASS_SFIO_VOLUME     0x2D
#define SMB2_FILE_INFO_CLASS_HARD_LINK       0x2E
#define SMB2_FILE_INFO_CLASS_UNKNOWN_2F      0x2F
#define SMB2_FILE_INFO_CLASS_NORMALIZED_NAME 0x30
#define SMB2_FILE_INFO_CLASS_UNKNOWN_31      0x31
#define SMB2_FILE_INFO_CLASS_UNKNOWN_33      0x33
#define SMB2_FILE_INFO_CLASS_UNKNOWN_34      0x34
#define SMB2_FILE_INFO_CLASS_UNKNOWN_35      0x35
#define SMB2_FILE_INFO_CLASS_STANDARD_LINK   0x36
#define SMB2_FILE_INFO_CLASS_UNKNOWN_37      0x37

typedef UCHAR SMB2_FS_INFO_CLASS;

#define SMB2_FS_INFO_CLASS_VOLUME    0x01
#define SMB2_FS_INFO_CLASS_SIZE      0x03
#define SMB2_FS_INFO_CLASS_DEVICE    0x04
#define SMB2_FS_INFO_CLASS_ATTRIBUTE 0x05
#define SMB2_FS_INFO_CLASS_CONTROL   0x06
#define SMB2_FS_INFO_CLASS_FULL_SIZE 0x07
#define SMB_FS_INFO_CLASS_OBJECTID   0x08

typedef UCHAR SMB2_SEC_INFO_CLASS;

#define SMB2_SEC_INFO_CLASS_BASIC    0x00

typedef ULONG SMB2_LOCK_FLAGS;

#define SMB2_LOCK_FLAGS_SHARED_LOCK      0x00000001
#define SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK   0x00000002
#define SMB2_LOCK_FLAGS_UNLOCK           0x00000004
#define SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY 0x00000010

typedef UCHAR SMB2_SEARCH_FLAGS;

#define SMB2_SEARCH_FLAGS_RESTART_SCAN        0x01
#define SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY 0x02
#define SMB2_SEARCH_FLAGS_INDEX_SPECIFIED     0x03
#define SMB2_SEARCH_FLAGS_REOPEN              0x10

typedef USHORT SMB2_NOTIFY_FLAGS;

#define SMB2_NOTIFY_FLAGS_WATCH_TREE          0x0001

typedef USHORT SMB2_CLOSE_FLAGS;

#define SMB2_CLOSE_FLAGS_GET_FILE_ATTRIBUTES  0x0001

#define SMB2_CONTEXT_NAME_DURABLE_HANDLE      "DHnQ"
#define SMB2_CONTEXT_NAME_MAX_ACCESS          "MxAc"
#define SMB2_CONTEXT_NAME_QUERY_DISK_ID       "QFid"
#define SMB2_CONTEXT_NAME_EXT_ATTRS           "ExtA"
#define SMB2_CONTEXT_NAME_SHADOW_COPY         "TWrp"
#define SMB2_CONTEXT_NAME_SEC_DESC            "SecD"
#define SMB2_CONTEXT_NAME_LEASE               "RqLs"

typedef USHORT SMB2_SHARE_TYPE;

#define SMB2_SHARE_TYPE_DISK       0x0001
#define SMB2_SHARE_TYPE_NAMED_PIPE 0x0002

typedef ULONG SMB2_SHARE_FLAGS;

#define SMB2_SHARE_FLAGS_DFS                   0x00000001
#define SMB2_SHARE_FLAGS_DFS_ROOT              0x00000002
#define SMB2_SHARE_FLAGS_CSC_NONE              0x00000030
#define SMB2_SHARE_FLAGS_CSC_MANUAL            0x00000000
#define SMB2_SHARE_FLAGS_CSC_AUTO              0x00000010
#define SMB2_SHARE_FLAGS_CSC_VDO               0x00000020
#define SMB2_SHARE_FLAGS_RESTRICT_EXCL_OPENS   0x00000100
#define SMB2_SHARE_FLAGS_FORCE_SHARED_DELETE   0x00000200
#define SMB2_SHARE_FLAGS_ALLOW_NS_CACHING      0x00000400
#define SMB2_SHARE_FLAGS_ACCESS_BASED_DIR_ENUM 0x00000800
#define SMB2_SHARE_FLAGS_FORCE_LEVELII_OPLOCK  0x00001000
#define SMB2_SHARE_FLAGS_ENABLE_HASH           0x00002000

typedef ULONG SMB2_SHARE_CAPABILITIES;

#define SMB2_SHARE_CAPABILITIES_DFS_AVAILABLE  0x00000008

typedef enum
{
    SMB2_CONTEXT_ITEM_TYPE_UNKNOWN = 0,
    SMB2_CONTEXT_ITEM_TYPE_DURABLE_HANDLE,
    SMB2_CONTEXT_ITEM_TYPE_MAX_ACCESS,
    SMB2_CONTEXT_ITEM_TYPE_QUERY_DISK_ID,
    SMB2_CONTEXT_ITEM_TYPE_EXT_ATTRS,
    SMB2_CONTEXT_ITEM_TYPE_SHADOW_COPY,
    SMB2_CONTEXT_ITEM_TYPE_SEC_DESC,
    SMB2_CONTEXT_ITEM_TYPE_LEASE

} SMB2_CONTEXT_ITEM_TYPE;

typedef USHORT SMB_SUB_COMMAND, *PSMB_SUB_COMMAND;

#define SMB_SUB_COMMAND_TRANS_SET_NAMED_PIPE_HANDLE_STATE   0x01
#define SMB_SUB_COMMAND_TRANS_RAW_READ_NAMED_PIPE           0x11
#define SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_HANDLE_STATE 0x21
#define SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_INFO         0x22
#define SMB_SUB_COMMAND_TRANS_PEEK_NAMED_PIPE               0x23
#define SMB_SUB_COMMAND_TRANS_TRANSACT_NAMED_PIPE           0x26
#define SMB_SUB_COMMAND_TRANS_RAW_WRITE_NAMED_PIPE          0x31
#define SMB_SUB_COMMAND_TRANS_WAIT_NAMED_PIPE               0x53
#define SMB_SUB_COMMAND_TRANS_CALL_NAMED_PIPE               0x54

#define SMB_SUB_COMMAND_TRANS2_OPEN2                        0x00
#define SMB_SUB_COMMAND_TRANS2_FIND_FIRST2                  0x01
#define SMB_SUB_COMMAND_TRANS2_FIND_NEXT2                   0x02
#define SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION         0x03
#define SMB_SUB_COMMAND_TRANS2_SET_FS_QUOTA                 0x04
#define SMB_SUB_COMMAND_TRANS2_QUERY_PATH_INFORMATION       0x05
#define SMB_SUB_COMMAND_TRANS2_SET_PATH_INFORMATION         0x06
#define SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION       0x07
#define SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION         0x08
#define SMB_SUB_COMMAND_TRANS2_FSCTL                        0x09
#define SMB_SUB_COMMAND_TRANS2_IOCTL2                       0x0A
#define SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_FIRST            0x0B
#define SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_NEXT             0x0C
#define SMB_SUB_COMMAND_TRANS2_CREATE_DIRECTORY             0x0D
#define SMB_SUB_COMMAND_TRANS2_SESSION_SETUP                0x0E
#define SMB_SUB_COMMAND_TRANS2_GET_DFS_REFERRAL             0x10
#define SMB_SUB_COMMAND_TRANS2_REPORT_DFS_INCONSISTENCY     0x11

#define SMB_SUB_COMMAND_NT_TRANSACT_CREATE              1
#define SMB_SUB_COMMAND_NT_TRANSACT_IOCTL               2
#define SMB_SUB_COMMAND_NT_TRANSACT_SET_SECURITY_DESC   3
#define SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE       4
#define SMB_SUB_COMMAND_NT_TRANSACT_RENAME              5
#define SMB_SUB_COMMAND_NT_TRANSACT_QUERY_SECURITY_DESC 6
#define SMB_SUB_COMMAND_NT_TRANSACT_QUERY_QUOTA         7
#define SMB_SUB_COMMAND_NT_TRANSACT_SET_QUOTA           8

typedef ULONG SMB_FILE_NOTIFY_CHANGE_FILTER;

#define SMB_FILE_NOTIFY_CHANGE_FILTER_FILE_NAME    0x00000001
#define SMB_FILE_NOTIFY_CHANGE_FILTER_DIR_NAME     0x00000002
#define SMB_FILE_NOTIFY_CHANGE_FILTER_NAME         0x00000003
#define SMB_FILE_NOTIFY_CHANGE_FILTER_ATTRIBUTES   0x00000004
#define SMB_FILE_NOTIFY_CHANGE_FILTER_SIZE         0x00000008
#define SMB_FILE_NOTIFY_CHANGE_FILTER_LAST_WRITE   0x00000010
#define SMB_FILE_NOTIFY_CHANGE_FILTER_LAST_ACCESS  0x00000020
#define SMB_FILE_NOTIFY_CHANGE_FILTER_CREATION     0x00000040
#define SMB_FILE_NOTIFY_CHANGE_FILTER_EA           0x00000080
#define SMB_FILE_NOTIFY_CHANGE_FILTER_SECURITY     0x00000100
#define SMB_FILE_NOTIFY_CHANGE_FILTER_STREAM_NAME  0x00000200
#define SMB_FILE_NOTIFY_CHANGE_FILTER_STREAM_SIZE  0x00000400
#define SMB_FILE_NOTIFY_CHANGE_FILTER_STREAM_WRITE 0x00000800

typedef ULONG SMB_FILE_NOTIFY_FILE_ACTION;

#define SMB_FILE_NOTIFY_FILE_ACTION_ADDED            0x00000001
#define SMB_FILE_NOTIFY_FILE_ACTION_REMOVED          0x00000002
#define SMB_FILE_NOTIFY_FILE_ACTION_MODIFIED         0x00000003
#define SMB_FILE_NOTIFY_FILE_ACTION_RENAMED_OLD_NAME 0x00000004
#define SMB_FILE_NOTIFY_FILE_ACTION_RENAMED_NEW_NAME 0x00000005
#define SMB_FILE_NOTIFY_FILE_ACTION_ADDED_STREAM     0x00000006
#define SMB_FILE_NOTIFY_FILE_ACTION_REMOVED_STREAM   0x00000007
#define SMB_FILE_NOTIFY_FILE_ACTION_MODIFIED_STREAM  0x00000008

typedef USHORT SMB_INFO_LEVEL, *PSMB_INFO_LEVEL;

#define SMB_INFO_STANDARD                   1
#define SMB_INFO_ALLOCATION                 1
#define SMB_INFO_QUERY_EA_SIZE              2
#define SMB_INFO_VOLUME                     2
#define SMB_INFO_QUERY_EAS_FROM_LIST        3
#define SMB_INFO_QUERY_ALL_EAS              4
#define SMB_INFO_IS_NAME_VALID              6
#define SMB_QUERY_FILE_BASIC_INFO         0x101
#define SMB_SET_FILE_BASIC_INFO           0x101
#define SMB_FIND_FILE_DIRECTORY_INFO      0x101
#define SMB_QUERY_FILE_STANDARD_INFO      0x102
#define SMB_QUERY_FS_VOLUME_INFO          0x102
#define SMB_SET_FILE_DISPOSITION_INFO     0x102
#define SMB_FIND_FILE_FULL_DIRECTORY_INFO 0x102
#define SMB_QUERY_FILE_EA_INFO            0x103
#define SMB_QUERY_FS_SIZE_INFO            0x103
#define SMB_FIND_FILE_NAMES_INFO          0x103
#define SMB_SET_FILE_ALLOCATION_INFO      0x103
#define SMB_QUERY_FILE_NAME_INFO          0x104
#define SMB_QUERY_FS_DEVICE_INFO          0x104
#define SMB_FIND_FILE_BOTH_DIRECTORY_INFO 0x104
#define SMB_SET_FILE_END_OF_FILE_INFO     0x104
#define SMB_QUERY_FS_ATTRIBUTE_INFO       0x105
#define SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO 0x105
#define SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO 0x106
#define SMB_QUERY_FILE_ALL_INFO           0x107
#define SMB_QUERY_FILE_ALT_NAME_INFO      0x108
#define SMB_QUERY_FILE_STREAM_INFO        0x109
#define SMB_QUERY_FILE_COMPRESSION_INFO   0x10B
#define SMB_QUERY_FILE_UNIX_BASIC         0x200
#define SMB_SET_FILE_UNIX_BASIC           0x200
#define SMB_QUERY_CIFS_UNIX_INFO          0x200
#define SMB_QUERY_FILE_UNIX_LINK          0x201
#define SMB_SET_FILE_UNIX_LINK            0x201
#define SMB_FIND_FILE_UNIX                0x202
#define SMB_SET_FILE_UNIX_HLINK           0x203
#define SMB_QUERY_MAC_FS_INFO             0x301
#define SMB_QUERY_FILE_BASIC_INFO_ALIAS   0x3EC /* alias for 0x101 */
#define SMB_SET_FILE_BASIC_INFO_ALIAS     0x3EC /* alias for 0x101 */
#define SMB_QUERY_FILE_STANDARD_INFO_ALIAS 0x3ED /* alias for 0x102 */
#define SMB_QUERY_FILE_EA_INFO_ALIAS      0x3EF /* alias for 0x103 */
#define SMB_QUERY_FILE_NAME_INFO_ALIAS    0x3F1 /* alias for 0x104 */
#define SMB_SET_FILE_RENAME_INFO          0x3F2
#define SMB_QUERY_FILE_ALL_INFO_ALIAS     0x3FA /* alias 0x107 */
#define SMB_SET_FILE_ALLOCATION_INFO_ALIAS 0x3FB /* alias 0x103 */
#define SMB_SET_FILE_END_OF_FILE_INFO_ALIAS 0x3FC /* alias 0x104 */
#define SMB_QUERY_FILE_ALT_NAME_INFO_ALIAS 0x3FD /* alias for 0x108 */
#define SMB_QUERY_FILE_STREAM_INFO_ALIAS  0x3FE /* alias for 0x109 */
#define SMB_QUERY_FILE_COMPRESSION_INFO_ALIAS 0x404 /* alias for 0x10B */
#define SMB_QUERY_FILE_INTERNAL_INFO      0x3EE
#define SMB_QUERY_FILE_ACCESS_INFO        0x3F0
#define SMB_QUERY_FILE_POSITION_INFO      0x3F6
#define SMB_QUERY_FILE_MODE_INFO          0x3F8
#define SMB_QUERY_FILE_ALIGNMENT_INFO     0x3F9
#define SMB_QUERY_FILE_NETWORK_OPEN_INFO  0x40A
#define SMB_QUERY_FILE_ATTRIBUTE_TAG_INFO 0x40B
#define SMB_SET_FILE_DISPOSITION_INFO_ALIAS 0x3F5
#define SMB_SET_FILE_POSITION_INFO        0x3F6
#define SMB_SET_FILE_MODE_INFO            0x3F8
#define SMB_SET_FILE_VALID_DATA_INFO      0x40F
#define SMB_QUERY_FS_VOLUME_INFO_ALIAS    0x3E9
#define SMB_QUERY_FS_SIZE_INFO_ALIAS      0x3EB
#define SMB_QUERY_FS_DEVICE_INFO_ALIAS    0x3EC
#define SMB_QUERY_FS_ATTRIBUTE_INFO_ALIAS 0x3ED
#define SMB_QUERY_FS_QUOTA_INFO           0x3EE
#define SMB_QUERY_FS_FULL_SIZE_INFO       0x3EF

typedef UCHAR LWIO_LOCK_TYPE;

#define LWIO_LOCK_TYPE_SHARED_LOCK         0x01
#define LWIO_LOCK_TYPE_OPLOCK_RELEASE      0x02
#define LWIO_LOCK_TYPE_CHANGE_LOCK_TYPE    0x04
#define LWIO_LOCK_TYPE_CANCEL_LOCK         0x08
#define LWIO_LOCK_TYPE_LARGE_FILES         0x10

typedef USHORT SMB_CSC_FLAGS;

/* SMB_V1 Client Side Caching Flags */
#define SMB_CSC_FLAG_MASK               0x000C
#define SMB_CSC_FLAG_CACHE_MANUAL_REINT 0x0000
#define SMB_CSC_FLAG_CACHE_AUTO_REINT   0x0004
#define SMB_CSC_FLAG_CACHE_VDO          0x0008
#define SMB_CSC_FLAG_CACHE_NONE         0x000C

typedef USHORT SMB_TREE_CONNECT_FLAGS;

#define SMB_TREE_CONNECT_DISCONNECT_TID    0x0001
#define SMB_TREE_CONNECT_EXT_SIGNATURES    0x0004
#define SMB_TREE_CONNECT_EXT_RESPONSE      0x0008

typedef enum
{
    FLAG_OBSOLETE_1     = 0x1,  /* Reserved for obsolescent requests
                                   LOCK_AND_READ, WRITE_AND_CLOSE */
    FLAG_RESERVED_1     = 0x2,  /* Reserved (must be zero) */
    FLAG_RESERVED_2     = 0x4,  /* Reserved (must be zero) */
    FLAG_CASELESS_PATHS = 0x8,  /* Caseless pathnames */
    FLAG_OBSOLETE_2     = 0x10, /* Canonicalized paths (obsolete) */
    FLAG_OBSOLETE_3     = 0x20, /* Reserved for obsolecent requests
                                   oplocks for SMB_COM_OPEN, etc. */
    FLAG_OBSOLETE_4     = 0x40, /* Reserved for obsolecent requests
                                   notifications for SMB_COM_OPEN, etc. */
    FLAG_RESPONSE       = 0x80, /* SMB is a response, not a request */
} HEADER_FLAGS;

typedef enum
{
    FLAG2_KNOWS_LONG_NAMES = 0x1,    /* If set in a request, the server may
                                        return long components in path names
                                        in the response. */
    FLAG2_KNOWS_EAS        = 0x2,    /* If set, the client is aware of
                                        extended attributes (EAs). */
    FLAG2_SECURITY_SIG     = 0x4,    /* If set, SMB is integrity checked. */
    FLAG2_RESERVED_1       = 0x8,    /* Reserved for future use */
    FLAG2_REQUIRE_SIG      = 0x10,   /* Undefined, must be zero */
    FLAG2_UNDEFINED_2      = 0x20,   /* Undefined, must be zero */
    FLAG2_IS_LONG_NAME     = 0x40,   /* If set, any path name in the request
                                        is a long name. */
    FLAG2_UNDEFINED_3      = 0x80,   /* Undefined, must be zero */
    FLAG2_UNDEFINED_4      = 0x100,  /* Undefined, must be zero */
    FLAG2_UNDEFINED_5      = 0x200,  /* Undefined, must be zero */
    FLAG2_UNDEFINED_6      = 0x400,  /* Undefined, must be zero */
    FLAG2_EXT_SEC          = 0x800,  /* If set, the client is aware of
                                        Extended Security negotiation. */
    FLAG2_DFS              = 0x1000, /* If set, any request pathnames in
                                        this SMB should be resolved in the
                                        Distributed File System. */
    FLAG2_PAGING_IO        = 0x2000, /* If set, indicates that a read will
                                        be permitted if the client does not
                                        have read permission but does have
                                        execute permission. This flag is
                                        only useful on a read request. */
    FLAG2_ERR_STATUS       = 0x4000, /* If set, specifies that the returned
                                        error code is a 32 bit error code
                                        in Status.Status. Otherwise the
                                        Status.DosError.ErrorClass and
                                        Status.DosError.Error fields contain
                                        the DOS-style error information.
                                        When passing NT status codes is
                                        negotiated, this flag should
                                        be set for every SMB. */
    FLAG2_UNICODE          = 0x8000, /* If set, any fields of datatype
                                        STRING in this SMB message are
                                        encoded as UNICODE. Otherwise, they
                                        are in ASCII. The character encoding
                                        for Unicode fields SHOULD be UTF-16
                                        (little endian). */
} HEADER_FLAGS2;

typedef enum
{
    /* Undefined bits must be set to zero by servers, and must be ignored by
       clients. */
    CAP_RAW_MODE         = 0x0001,  /* The server supports SMB_COM_READ_RAW
                                       and SMB_COM_WRITE_RAW (obsolescent) */
    CAP_MPX_MODE         = 0x0002,  /* The server supports SMB_COM_READ_MPX
                                       and SMB_COM_WRITE_MPX (obsolescent) */
    CAP_UNICODE          = 0x0004,  /* The server supports UNICODE strings */
    CAP_LARGE_FILES      = 0x0008,  /* The server supports large files with 64
                                       bit offsets */
    CAP_NT_SMBS          = 0x0010,  /* The server supports the SMBs particular
                                       to the NT LM 0.12 dialect. Implies
                                       CAP_NT_FIND. */
    CAP_RPC_REMOTE_APIS  = 0x0020,  /* The server supports remote admin API
                                       requests via DCE RPC */
    CAP_STATUS32         = 0x0040,  /* The server can respond with 32 bit status
                                       codes in Status.Status */
    CAP_LEVEL_II_OPLOCKS = 0x0080, /* The server supports level 2 oplocks
                                       Capability Name Encoding Meaning */
    CAP_LOCK_AND_READ    = 0x0100,  /* The server supports the SMB,
                                       SMB_COM_LOCK_AND_READ */
    CAP_NT_FIND          = 0x0200,  /* Reserved */
    CAP_DFS              = 0x1000,  /* The server is DFS aware */
    CAP_INFOLEVEL_PASSTHRU = 0x2000, /* The server supports NT information
                                        level requests passing through */
    CAP_LARGE_READX      = 0x4000,  /* The server supports large
                                       SMB_COM_READ_ANDX (up to 64k) */
    CAP_LARGE_WRITEX     = 0x8000,  /* The server supports large
                                       SMB_COM_WRITE_ANDX (up to 64k) */
    CAP_UNIX             = 0x00800000, /* The server supports CIFS Extensions
                                          for UNIX. */
    CAP_RESERVED         = 0x02000000, /* Reserved for future use */
    CAP_BULK_TRANSFER    = 0x20000000, /* The server supports SMB_BULK_READ,
                                          SMB_BULK_WRITE (should be 0, no known
                                          implementations) */
    CAP_COMPRESSED_DATA  = 0x40000000, /* The server supports compressed data
                                          transfer (BULK_TRANSFER capability is
                                          required to support compressed data
                                          transfer). */
    CAP_EXTENDED_SECURITY = 0x80000000 /* The server supports extended security
                                          exchanges */
} CAPABILITIES;

typedef USHORT SMB_DEVICE_STATE;

#define SMB_DEVICE_STATE_NO_EAS         0x1
#define SMB_DEVICE_STATE_NO_SUBSTREAMS  0x2
#define SMB_DEVICE_STATE_NO_REPARSE_TAG 0x4

typedef UCHAR SMB_BUFFER_FORMAT;

#define SMB_BUFFER_FORMAT_DATA_BLOCK    0x1
#define SMB_BUFFER_FORMAT_DIALECT       0x2 /* null terminated string */
#define SMB_BUFFER_FORMAT_PATHNAME      0x3 /* null terminated string */
#define SMB_BUFFER_FORMAT_ASCII         0x4 /* null terminated string */
#define SMB_BUFFER_FORMAT_VARIABLE      0x5

#define SMB_V1_GET_PROCESS_ID(pHdr) \
    ((((ULONG)pHdr->extra.pidHigh) << 16) | ((ULONG)pHdr->pid))

typedef struct
{
    uchar8_t        smb[4];     /* Contains 0xFF 'SMB' */
    uint8_t         command;    /* Command code */
    uint32_t        error;      /* Error code */
    uint8_t         flags;      /* Flags */
    uint16_t        flags2;     /* More flags */
    union
    {
        uint16_t    pad[6];     /* Ensure section is 12 bytes long */
        struct
        {
            uint16_t    pidHigh;                /* High part of PID */
            uint8_t     securitySignature[8];   /* Reserved for MAC */
        } extra;
    };
    uint16_t        tid;        /* Tree identifier */
    uint16_t        pid;        /* Caller's process ID opaque for cilent use */
    uint16_t        uid;        /* User ID */
    uint16_t        mid;        /* Multiplex ID */
    uint8_t         wordCount;  /* Count of parameter words */

    /* AndX or message specific parameters immediately follow */
} __attribute__((__packed__)) SMB_HEADER, *PSMB_HEADER;

typedef struct
{
	uchar8_t        smb[4];     /* Contains 0xFE 'SMB' */
	uint16_t        usHeaderLen;
	uint16_t        usEpoch;
	uint32_t        error;
	uint16_t        command;
	uint16_t        usCredits;
	uint32_t        ulFlags;
	uint32_t        ulChainOffset;
	uint64_t        ullCommandSequence;
	uint32_t        ulPid;
	uint32_t        ulTid;
	uint64_t        ullSessionId;
	uchar8_t        signature[16];
} __attribute__((__packed__)) SMB2_HEADER, *PSMB2_HEADER;

typedef struct
{
    uint8_t     andXCommand;    /* Secondary (X) command; 0xFF = none */
    uint8_t     andXReserved;   /* Reserved (must be 0) */
    uint16_t    andXOffset;     /* Offset to next command wordCount */

    /* Message specific parameters immediately follow */
} __attribute__((__packed__)) ANDX_HEADER, *PANDX_HEADER;

typedef struct
{
    uint32_t    len;

    /* SMB packet immediately follows */
}  __attribute__((__packed__))  NETBIOS_HEADER;

typedef enum
{
    SMB_PROTOCOL_VERSION_UNKNOWN = 0,
	SMB_PROTOCOL_VERSION_1,
	SMB_PROTOCOL_VERSION_2
} SMB_PROTOCOL_VERSION;

typedef enum
{
    SMB_PROTOCOL_DIALECT_UNKNOWN = 0,
	SMB_PROTOCOL_DIALECT_NTLM_0_12,
	SMB_PROTOCOL_DIALECT_SMB_2_0,
	SMB_PROTOCOL_DIALECT_SMB_2_1,

} SMB_PROTOCOL_DIALECT;

typedef struct
{
    LONG            refCount;

    NETBIOS_HEADER *pNetBIOSHeader;
    UINT8           netbiosOpcode;

    SMB_PROTOCOL_VERSION protocolVer;
    union
    {
    	PSMB_HEADER  pSMBHeader;
    	PSMB2_HEADER pSMB2Header;
    };

    ANDX_HEADER    *pAndXHeader; /* If NULL, no AndX */

    uint8_t        *pParams;     /* Pointer to start of message specific
                                    parameters */
    uint8_t        *pData;       /* Pointer to start of message data portion;
                                    if NULL, data may still reside in the
                                    socket */
    uint8_t        *pRawBuffer;  /* Pointer to the raw buffer, equals
                                    pNetBIOSHeader */
    size_t          bufferLen;   /* Number of bytes allocated from buffer */
    uint32_t        bufferUsed;  /* Number of bytes available/needed from
                                    buffer */
    uint32_t        sequence;    /* Sequence number */
    uint8_t         allowSignature; /* Whether to allow signing for this packet */
    uint8_t         haveSignature; /* Whether packet has signature */
} SMB_PACKET, *PSMB_PACKET;

typedef enum
{
    /* The extended file attributes is a 32 bit value composed of attributes
       and flags. */

    /* Any combination of the following attributes is acceptable, except all
       other file attributes override FILE_ATTR_NORMAL: */

    /* The file has not been archived since it was last modified. Applications
       use this attribute to mark files for backup or removal. */
    ATTR_ARCHIVE    = 0x020,

    /* The file or directory is compressed. For a file, this means that all of
       the data in the file is compressed. For a directory, this means that
       compression is the default for newly created files and subdirectories.
       The state of the attribute ATTR_COMPRESSED does not affect how data is
       read or written to the file or directory using the SMB operations. The
       attribute only indicates how the server internally stores the data. */
    ATTR_COMPRESSED = 0x800,

    /* The file has no other attributes set. This attribute is valid only if
       used alone. */
    ATTR_NORMAL     = 0x080,

    /* The file is hidden. It is not to be included in an ordinary directory
       listing. */
    ATTR_HIDDEN     = 0x002,

    /* The file is read only. Applications can read the file but cannot write
       to it or delete it. */
    ATTR_READONLY   = 0x001,
    ATTR_TEMPORARY  = 0x100, /* The file is temporary. */
    ATTR_DIRECTORY  = 0x010, /* The file is a directory. */

    /* The file is part of or is used exclusively by the operating system. */
    ATTR_SYSTEM     = 0x004,

    /* Any combination of the following flags is acceptable: */

    /* Instructs the operating system to write through any intermediate cache
     * and go directly to the file. The operating system can still cache write
       operations, but cannot lazily flush them. */
    WRITE_THROUGH   = 0x80000000,

    /* Requests the server to open the file with no intermediate buffering or
       caching; the server is not obliged to honor the request. An application
       must meet certain requirements when working with files opened with
       FILE_FLAG_NO_BUFFERING. File access must begin at offsets within the
       file that are integer multiples of the volume's sector size; and must be
       for numbers of bytes that are integer multiples of the volume's sector
       size. For example, if the sector size is 512 bytes, an application can
       request reads and writes of 512, 1024, or 2048 bytes, but not of 335,
       981, or 7171 bytes.*/
    NO_BUFFERING    = 0x20000000,

    /* Indicates that the application intends to access the file randomly. The
       server MAY use this flag to optimize file caching. */
    RANDOM_ACCESS   = 0x10000000,

    /* Indicates that the file is to be accessed sequentially from beginning to
       end. Windows uses this flag to optimize file caching. If an application
       moves the file pointer for random access, optimum caching may not occur;
       however, correct operation is still guaranteed. Specifying this flag can
       increase performance for applications that read large files using
       sequential access. Performance gains can be even more noticeable for
       applications that read large files mostly sequentially, but occasionally
       skip over small ranges of bytes.*/
    SEQUENTIAL_SCAN  = 0x08000000,

    /* Requests that the server is delete the file immediately after all of its
       handles have been closed. */
    DELETE_ON_CLOSE  = 0x04000000,

    /* Indicates that the file is being opened or created for a backup or
       restore operation. The server SHOULD allow the client to override normal
       file security checks, provided it has the necessary permission to do
       so. */
    BACKUP_SEMANTICS = 0x02000000,

    /* Indicates that the file is to be accessed according to POSIX rules. This
       includes allowing multiple files with names differing only in case, for
       file systems that support such naming. (Use care when using this option
       because files created with this flag may not be accessible by
       applications written for MS-DOS, Windows 3.x, or Windows NT.) */
    POSIX_SEMANTICS  = 0x01000000

} EXT_FILE_ATTRIBUTES;

typedef enum
{
    /* The shareAccess field specifies how the file can be shared. This
       parameter must be some combination of the following values: */

    FILE_NO_SHARE     = 0x00000000, /* Prevents the file from being shared. */
#if 0
    FILE_SHARE_READ   = 0x00000001, /* Other open operations can be performed
                                       on the file for read access. */
    FILE_SHARE_WRITE  = 0x00000002, /* Other open operations can be performed
                                       on the file for write access. */
    FILE_SHARE_DELETE = 0x00000004  /* Other open operations can be performed
                                       on the file for delete access. */
#endif
} SHARE_ACCESS;

#if 0
typedef enum
{
    /* Indicates that if the file already exists then it should be superseded
       by the specified file.  If it does not already exist then it should be
       created.*/
    FILE_SUPERSEDE    = 0x00000000,

    /* Indicates that if the file already exists it should be opened rather
       than creating a new file.  If the file does not already exist then the
       operation should fail. */
    FILE_OPEN         = 0x00000001,

    /* Indicates that if the file already exists then the operation should
       fail.  If the file does not already exist then it should be created. */
    FILE_CREATE       = 0x00000002,

    /* Indicates that if the file already exists, it should be opened.  If the
       file does not already exist then it should be created. */
    FILE_OPEN_IF      = 0x00000003,

    /* Indicates that if the file already exists it should be opened and
       overwritten.  If the file does not already exist then the operation
       should fail. */
    FILE_OVERWRITE    = 0x00000004,

    /* Indicates that if the file already exists it should be opened and
       overwritten.  If the file does not already exist then it should be
       created.*/
    FILE_OVERWRITE_IF = 0x00000005
} CREATE_DISPOSITION;
#endif

typedef enum
{
    /* The impersonationLevel parameter can contain one or more of the
       following values: */
    SECURITY_ANONYMOUS      = 0,
    SECURITY_IDENTIFICATION = 1,
    SECURITY_IMPERSONATION  = 2,
    SECURITY_DELEGATION     = 3,

} IMPERSONATION_LEVEL;

typedef enum
{
    /* Specifies that the security tracking mode is dynamic. If this flag is
       not specified, Security Tracking Mode is static. */
    SECURITY_CONTEXT_TRACKING = 0x00040000,

    /* Specifies that only the enabled aspects of the client's security context
       are available to the server.  If this flag is not specified, all aspects
       of the client's security context are available. This flag allows the
       client to limit the groups and privileges that a server can use while
       impersonating the client. */
    SECURITY_EFFECTIVE_ONLY   = 0x00080000
} SECURITY_FLAGS;

typedef struct
{
    USHORT usDay   : 5; /* 1 to 31 */
    USHORT usMonth : 4; /* 1 to 12 */
    USHORT usYear  : 7; /* 0 to 119 yields years 1980 to 2099 */
} __attribute__((__packed__)) SMB_DATE, *PSMB_DATE;

typedef struct
{
    USHORT TwoSeconds : 5; /* 0 to 29 */
    USHORT Minutes    : 6; /* 0 to 59 */
    USHORT Hours      : 5; /* 0 to 23 */
} __attribute__((__packed__)) SMB_TIME, *PSMB_TIME;

typedef struct {

    USHORT   usPid;
    ULONG    ulOffset;
    ULONG    ulLength;

} __attribute__((__packed__))  LOCKING_ANDX_RANGE, *PLOCKING_ANDX_RANGE;

typedef struct {

    USHORT   usPid;
    USHORT   usPad;
    ULONG    ulOffsetHigh;
    ULONG    ulOffsetLow;
    ULONG    ulLengthHigh;
    ULONG    ulLengthLow;

} __attribute__((__packed__))  LOCKING_ANDX_RANGE_LARGE_FILE,
                              *PLOCKING_ANDX_RANGE_LARGE_FILE;

typedef struct {

    ULONG    ulFileSystemId;
    ULONG    ulNumSectorsPerAllocationUnit;
    ULONG    ulNumAllocationUnits;
    ULONG    ulNumUnitsAvailable;
    USHORT   usNumBytesPerSector;

} __attribute__((__packed__)) SMB_FS_INFO_ALLOCATION, *PSMB_FS_INFO_ALLOCATION;

typedef struct
{
    USHORT usFid;
    SMB_INFO_LEVEL infoLevel;
} __attribute__((__packed__)) SMB_QUERY_FILE_INFO_HEADER,
                             *PSMB_QUERY_FILE_INFO_HEADER;

typedef struct
{
    SMB_INFO_LEVEL infoLevel;
    ULONG ulReserved;
} __attribute__((__packed__)) SMB_QUERY_PATH_INFO_HEADER,
                             *PSMB_QUERY_PATH_INFO_HEADER;

typedef struct
{
    SMB_INFO_LEVEL infoLevel;
} __attribute__((__packed__)) SMB_QUERY_FS_INFO_HEADER,
                             *PSMB_QUERY_FS_INFO_HEADER;

typedef struct
{
    USHORT usFid;
    SMB_INFO_LEVEL infoLevel;
    USHORT usReserved;
} __attribute__((__packed__)) SMB_SET_FILE_INFO_HEADER,
                             *PSMB_SET_FILE_INFO_HEADER;

typedef struct
{
    SMB_INFO_LEVEL infoLevel;
    ULONG reserved;
    WCHAR pwszPath[];
} __attribute__((__packed__)) SMB_SET_PATH_INFO_HEADER,
                             *PSMB_SET_PATH_INFO_HEADER;

typedef struct
{
    USHORT usFid;
    USHORT usFlags;
    WCHAR pwszFileName[];
} __attribute__((__packed__)) SMB_TRANSACT_RENAME_HEADER,
                             *PSMB_TRANSACT_RENAME_HEADER;

typedef struct
{
    USHORT usSearchAttrs;
    USHORT usSearchCount;
    USHORT usFlags;
    SMB_INFO_LEVEL infoLevel;
    ULONG ulSearchStorageType;
    WCHAR pwszSearchPattern[];
} __attribute__((__packed__)) SMB_FIND_FIRST2_REQUEST_PARAMETERS,
                             *PSMB_FIND_FIRST2_REQUEST_PARAMETERS;

typedef struct _SMB_FIND_FIRST2_RESPONSE_PARAMETERS
{
    USHORT usSearchId;
    USHORT usSearchCount;
    USHORT usEndOfSearch;
    USHORT usEaErrorOffset;
    USHORT usLastNameOffset;
} __attribute__((__packed__)) SMB_FIND_FIRST2_RESPONSE_PARAMETERS,
                             *PSMB_FIND_FIRST2_RESPONSE_PARAMETERS;

typedef struct
{
    USHORT usSearchId;
    USHORT usSearchCount;
    SMB_INFO_LEVEL infoLevel;
    ULONG ulResumeKey;
    USHORT usFlags;
    WCHAR pwszFileName[];
} __attribute__((__packed__)) SMB_FIND_NEXT2_REQUEST_PARAMETERS,
                             *PSMB_FIND_NEXT2_REQUEST_PARAMETERS;

typedef struct
{
    USHORT usSearchCount;
    USHORT usEndOfSearch;
    USHORT usEaErrorOffset;
    USHORT usLastNameOffset;
} __attribute__((__packed__)) SMB_FIND_NEXT2_RESPONSE_PARAMETERS,
                             *PSMB_FIND_NEXT2_RESPONSE_PARAMETERS;

#define SMB_CREATE_REQUEST_EXT_RESPONSE 0x00000010

typedef struct {
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint8_t  reserved;           /* Reserved (must be 0) */
    uint16_t nameLength;         /* Length of Name[] in bytes */
    uint32_t flags;              /* Create bit set:
                                    0x02 - Request an oplock
                                    0x04 - Request a batch oplock
                                    0x08 - Target of open must be directory
                                    0x10 - Request an extended response */
    uint32_t rootDirectoryFid;   /* If non-zero, open is relative to this
                                    directory */
    uint32_t desiredAccess;      /* Access desired (See Section 3.8 for an
                                    explanation of this field) */
    int64_t  allocationSize;     /* Initial allocation size */
    uint32_t extFileAttributes;  /* File attributes */
    uint32_t shareAccess;        /* Type of share access */
    uint32_t createDisposition;  /* Action if file does/does not exist */
    uint32_t createOptions;      /* Options to use if creating a file */
    uint32_t impersonationLevel; /* Security QOS information */
    uint8_t  securityFlags;      /* Security tracking mode flags:
                                        0x1 - SECURITY_CONTEXT_TRACKING
                                        0x2 - SECURITY_EFFECTIVE_ONLY */
    uint16_t byteCount;          /* Length of byte parameters */
    wchar16_t Name[];            /* File to open or create */

    /* Name immediately follows */
}  __attribute__((__packed__))  CREATE_REQUEST_HEADER, *PCREATE_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer         */
    USHORT   usFlags;
    USHORT   usDesiredAccess;
    USHORT   usSearchAttributes;
    USHORT   usFileAttributes;
    ULONG    ulCreationTime;
    USHORT   usOpenFunction;
    ULONG    ulAllocationSize;
    ULONG    ulReserved[2];
    USHORT   usByteCount;

    // UCHAR  ucBufferFormat;
    // STRING pwszFileName;

} __attribute__((__packed__)) OPEN_REQUEST_HEADER, *POPEN_REQUEST_HEADER;

typedef struct
{
    USHORT   usFid;
    USHORT   usFileAttributes;
    ULONG    ulLastWriteTime;
    ULONG    ulDataSize; /* file size */
    USHORT   usGrantedAccess;
    USHORT   usFileType;
    USHORT   usDeviceState;
    USHORT   usOpenAction;
    ULONG    ulServerFid;
    USHORT   usReserved;
    USHORT   usByteCount;

} __attribute__((__packed__)) OPEN_RESPONSE_HEADER, *POPEN_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint8_t  oplockLevel;       /* The oplock level granted:
                                        0 - No oplock granted
                                        1 - Exclusive oplock granted
                                        2 - Batch oplock granted
                                        3 - Level II oplock granted */
    uint16_t fid;               /* The file ID */
    uint32_t createAction;      /* The action taken */
    int64_t  creationTime;      /* The time the file was created */
    int64_t  lastAccessTime;    /* The time the file was accessed */
    int64_t  lastWriteTime;     /* The time the file was last written */
    int64_t  changeTime;        /* The time the file was last changed */
    uint32_t extFileAttributes; /* The file attributes */
    int64_t  allocationSize;    /* The number of byes allocated */
    int64_t  endOfFile;         /* The end of file offset */
    uint16_t fileType;
    uint16_t deviceState;       /* State of IPC device (e.g. pipe) */
    uint8_t  isDirectory;       /* TRUE if this is a directory */
    uint16_t byteCount;         /* = 0 */

}  __attribute__((__packed__))  CREATE_RESPONSE_HEADER,
                               *PCREATE_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint8_t  oplockLevel;       /* The oplock level granted:
                                        0 - No oplock granted
                                        1 - Exclusive oplock granted
                                        2 - Batch oplock granted
                                        3 - Level II oplock granted */
    uint16_t fid;               /* The file ID */
    uint32_t createAction;      /* The action taken */
    int64_t  creationTime;      /* The time the file was created */
    int64_t  lastAccessTime;    /* The time the file was accessed */
    int64_t  lastWriteTime;     /* The time the file was last written */
    int64_t  changeTime;        /* The time the file was last changed */
    uint32_t extFileAttributes; /* The file attributes */
    int64_t  allocationSize;    /* The number of byes allocated */
    int64_t  endOfFile;         /* The end of file offset */
    uint16_t fileType;
    uint16_t deviceState;       /* State of IPC device (e.g. pipe) */
    uint8_t  isDirectory;       /* TRUE if this is a directory */
    BYTE     volumeGUID[16];    /* GUID which uniquely identifies the volume */
    uint64_t fileID;            /* 64-bit value that identifies this file on
                                   a volume */
    uint32_t MaximalAccessRights; /* The maximum access rights that the user
                                     opening the file has been granted for
                                     this open */
    uint32_t GuestMaximalAccessRights;
    uint16_t byteCount;         /* = 0 */

}  __attribute__((__packed__))  CREATE_RESPONSE_EXT_HEADER,
                               *PCREATE_RESPONSE_EXT_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  maxBufferSize;   /* Client's maximum buffer size */
    uint16_t  maxMpxCount;     /* Actual maximum multiplexed pending
                                * requests */
    uint16_t  vcNumber;        /* 0 = first (only), nonzero=additional VC
                                * number */
    uint32_t  sessionKey;      /* Session key (valid iff VcNumber != 0) */
    uint16_t  securityBlobLength; /* Length of opaque security blob */
    uint32_t  reserved;        /* Must be 0 */
    uint32_t  capabilities;    /* Client capabilities */
    uint16_t  byteCount;       /* Count of data bytes; min = 0 */

    /* Data immediately follows */
}  __attribute__((__packed__))  SESSION_SETUP_REQUEST_HEADER_WC_12;


typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  maxBufferSize;   /* Client's maximum buffer size */
    uint16_t  maxMpxCount;     /* Actual maximum multiplexed pending
                                * requests */
    uint16_t  vcNumber;        /* 0 = first (only), nonzero=additional VC
                                * number */
    uint32_t  sessionKey;      /* Session key (valid iff VcNumber != 0) */
    uint16_t  lmResponseLength;/* Length of LMv2 value */
    uint16_t  ntResponseLength;/* Length of NTLMv2 value */
    uint32_t  reserved;        /* Must be 0 */
    uint32_t  capabilities;    /* Client capabilities */
    uint16_t  byteCount;       /* Count of data bytes; min = 0 */

    /* Data immediately follows */
}  __attribute__((__packed__))  SESSION_SETUP_REQUEST_HEADER_WC_13;


typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t    action;             /* Request mode: */
                                    /*      bit0 = logged in as GUEST */
    uint16_t    securityBlobLength; /* Length of Security Blob that */
                                    /* follows in a later field */
    uint16_t    byteCount;          /* Count of data bytes */

     /* Data immediately follows */
}  __attribute__((__packed__)) SESSION_SETUP_RESPONSE_HEADER_WC_4,
                              *PSESSION_SETUP_RESPONSE_HEADER_WC_4;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t    action;             /* Request mode: */
                                    /*      bit0 = logged in as GUEST */
    uint16_t    byteCount;          /* Count of data bytes */

     /* Data immediately follows */
}  __attribute__((__packed__)) SESSION_SETUP_RESPONSE_HEADER_WC_3,
                              *PSESSION_SETUP_RESPONSE_HEADER_WC_3;

typedef struct
{
    uint16_t byteCount;
#if 0
    struct {
        UCHAR BufferFormat;
        UCHAR DialectName[];
    } Dialects[];
#endif
} __attribute__((__packed__)) NEGOTIATE_REQUEST_HEADER,
                             *PNEGOTIATE_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */

    uint16_t dialectIndex;       /* Index of selected dialect */
    uint8_t  securityMode;       /* Security mode:
                                  *      bit 0: 0 = share, 1 = user
                                  *      bit 1: 1 = encrypt passwords
                                  *      bit 2: 1 = Security Signatures
                                  *          (SMB sequence numbers) enabled
                                  *      bit 3: 1 = Security Signatures
                                  *          (SMB sequence numbers) required */
    uint16_t maxMpxCount;         /* Max pending outstanding requests */
    uint16_t maxNumberVcs;        /* Max VCs between client and server */
    uint32_t maxBufferSize;       /* Max transmit buffer size */
    uint32_t maxRawSize;          /* Maximum raw buffer size */
    uint32_t sessionKey;          /* Unique token identifying this session */
    uint32_t capabilities;        /* Server capabilities */
    uint32_t systemTimeLow;       /* System (UTC) time of the server (low) */
    uint32_t systemTimeHigh;      /* System (UTC) time of the server (high) */
     int16_t serverTimeZone;      /* Time zone of server (minutes from UTC) */
    uint8_t  encryptionKeyLength; /* Length of encryption key */

    uint16_t byteCount;           /* Count of data bytes */

    /* Data immediately follows */
}  __attribute__((__packed__))  NEGOTIATE_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */

    uint16_t dialectIndex;       /* Index of selected dialect */
    uint16_t byteCount;           /* Count of data bytes */

}  __attribute__((__packed__))  NEGOTIATE_INVALID_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  flags;            /* Addiitonal information */
                                /*      bit 0 set = disconnect TID */

    /* The password field is obsolete in modern dialects */
    uint16_t  passwordLength;   /* Length of password */

    uint16_t  byteCount;       /* Count of data bytes; min = 3 */

    /* Data immediately follows */
}  __attribute__((__packed__))  TREE_CONNECT_REQUEST_HEADER,
                               *PTREE_CONNECT_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  optionalSupport;  /* Optional support bits */
                                /*      SMB_SUPPORT_SEARCH_BITS = 0x0001 */
                                /* Exclusive search bits */
                                /*      ("MUST HAVE BITS") supported */
                                /*      SMB_SHARE_IS_IN_DFS = 0x0002 */
    uint16_t  byteCount;        /* Count of data bytes; min = 3 */

    /* Data immediately follows */
}  __attribute__((__packed__))  TREE_CONNECT_RESPONSE_HEADER,
                               *PTREE_CONNECT_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  optionalSupport;  /* Optional support bits */
                                /*      SMB_SUPPORT_SEARCH_BITS = 0x0001 */
                                /* Exclusive search bits */
                                /*      ("MUST HAVE BITS") supported */
                                /*      SMB_SHARE_IS_IN_DFS = 0x0002 */
    uint32_t  maximalShareAccessMask;
    uint32_t  guestMaximalShareAccessMask;
    uint16_t  byteCount;        /* Count of data bytes; min = 3 */

    /* Data immediately follows */
}  __attribute__((__packed__))  TREE_CONNECT_EXT_RESPONSE_HEADER,
                               *PTREE_CONNECT_EXT_RESPONSE_HEADER;

typedef struct
{
    uint16_t byteCount;
} __attribute__((__packed__)) TREE_DISCONNECT_RESPONSE_HEADER,
                             *PTREE_DISCONNECT_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and command are handled at a higher layer */
    uint16_t totalParameterCount;  /* Total parameter bytes being sent */
    uint16_t totalDataCount;       /* Total data bytes being sent */
    uint16_t maxParameterCount;    /* Max parameter bytes to return */
    uint16_t maxDataCount;         /* Max data bytes to return */
    uint8_t  maxSetupCount;        /* Max setup words to return */
    uint8_t  reserved;
    uint16_t flags;                /* Additional information:
                                         bit 0 - Disconnect TID */
    uint32_t timeout;
    uint16_t reserved2;
    uint16_t parameterCount;       /* Parameter bytes sent this buffer */
    uint16_t parameterOffset;      /* Offset (from header start) to
                                         parameters */
    uint16_t dataCount;            /* Data bytes sent this buffer */
    uint16_t dataOffset;           /* Offset from header start to data */
    uint8_t  setupCount;           /* Count of setup words */
    uint8_t  reserved3;            /* Reserved (pad above to word boundary) */

    /* Setup words immediately follow */
}  __attribute__((__packed__))  TRANSACTION_REQUEST_HEADER,
                               *PTRANSACTION_REQUEST_HEADER;

typedef struct
{
    /* wordCount and command are handled at a higher layer */

    UCHAR    ucMaxSetupCount;
    USHORT   usReserved;
    ULONG    ulTotalParameterCount;  /* Total parameter bytes being sent */
    ULONG    ulTotalDataCount;       /* Total data bytes being sent */
    ULONG    ulMaxParameterCount;
    ULONG    ulMaxDataCount;
    ULONG    ulParameterCount;
    ULONG    ulParameterOffset;
    ULONG    ulDataCount;
    ULONG    ulDataOffset;
    UCHAR    ucSetupCount;
    USHORT   usFunction;
    /* Setup words immediately follow */
}  __attribute__((__packed__))  NT_TRANSACTION_REQUEST_HEADER,
                               *PNT_TRANSACTION_REQUEST_HEADER;

typedef struct
{
    /* wordCount and command are handled at a higher layer */

    UCHAR ucReserved[3];
    ULONG ulTotalParameterCount;
    ULONG ulTotalDataCount;
    ULONG ulParameterCount;
    ULONG ulParameterOffset;
    ULONG ulParameterDisplacement;
    ULONG ulDataCount;
    ULONG ulDataOffset;
    ULONG ulDataDisplacement;
    UCHAR ucSetupCount;

    /* Setup words immediately follow */

} __attribute__((__packed__))  NT_TRANSACTION_SECONDARY_RESPONSE_HEADER,
                              *PNT_TRANSACTION_SECONDARY_RESPONSE_HEADER;

/* @todo: is this ever sent? */
typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    uint16_t byteCount;            /* Count of data bytes */
}  __attribute__((__packed__))  TRANSACTION_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    uint16_t totalParameterCount;     /* Total parameter bytes being sent */
    uint16_t totalDataCount;          /* Total data bytes being sent */
    uint16_t parameterCount;          /* Parameter bytes sent this buffer */
    uint16_t parameterOffset;         /* Offset from header start to params. */
    uint16_t parameterDisplacement;   /* Displacement of parameter bytes */
    uint16_t dataCount;               /* Data bytes sent this buffer */
    uint16_t dataOffset;              /* Offset from header start to data */
    uint16_t dataDisplacement;        /* Displacement of these data bytes */
    uint16_t fid;                     /* FID for handle based requests, else
                                         0xFFFF.  This field is present only
                                         if this is an SMB_COM_TRANSACTION2
                                         request. */
    uint16_t byteCount;               /* Count of data bytes */

    /* Parameters and data follow */
}  __attribute__((__packed__))  TRANSACTION_SECONDARY_REQUEST_HEADER,
                               *PTRANSACTION_SECONDARY_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    uint16_t totalParameterCount;   /* Total parameter bytes being sent */
    uint16_t totalDataCount;        /* Total data bytes being sent */
    uint16_t reserved;
    uint16_t parameterCount;        /* Parameter bytes sent this buffer */
    uint16_t parameterOffset;       /* Offset from header start to params. */
    uint16_t parameterDisplacement; /* Displacement of these parameter bytes */
    uint16_t dataCount;             /* Data bytes sent this buffer */
    uint16_t dataOffset;            /* Offset (from header start) to data */
    uint16_t dataDisplacement;      /* Displacement of these data bytes */
    uint8_t  setupCount;            /* Count of setup words */
    uint8_t  reserved2;             /* Reserved (pad above to word boundary) */

    /* Parameters and data follow */
}  __attribute__((__packed__))  TRANSACTION_SECONDARY_RESPONSE_HEADER,
                               *PTRANSACTION_SECONDARY_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;           /* File handle */
    uint32_t offset;        /* Offset in file to begin read */
    uint16_t maxCount;      /* Max number of bytes to return */
    uint16_t minCount;      /* 0 = non-blocking named pipe read */
    uint32_t maxCountHigh;  /* High 16 bits of MaxCount if CAP_LARGE_READX;
                               else MUST BE ZERO */
    uint16_t remaining;     /* Reserved for obsolescent requests */
    uint16_t byteCount;     /* Count of data bytes = 0 */
}  __attribute__((__packed__))  READ_ANDX_REQUEST_HEADER_WC_10,
                               *PREAD_ANDX_REQUEST_HEADER_WC_10;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;           /* File handle */
    uint32_t offset;        /* Offset in file to begin read */
    uint16_t maxCount;      /* Max number of bytes to return */
    uint16_t minCount;      /* 0 = non-blocking named pipe read */
    uint32_t maxCountHigh;  /* High 16 bits of MaxCount if CAP_LARGE_READX;
                               else MUST BE ZERO */
    uint16_t remaining;     /* Reserved for obsolescent requests */
    uint32_t offsetHigh;    /* Upper 32 bits of offset
                               (only if wordCount is 12) */
    uint16_t byteCount;     /* Count of data bytes = 0 */
}  __attribute__((__packed__))  READ_ANDX_REQUEST_HEADER_WC_12,
                               *PREAD_ANDX_REQUEST_HEADER_WC_12;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t remaining;         /* Reserved -- must be -1 */
    uint16_t dataCompactionMode;
    uint16_t reserved;          /* Reserved (must be 0) */
    uint16_t dataLength;        /* Number of data bytes (min = 0) */
    uint16_t dataOffset;        /* Offset (from header start) to data */
    uint16_t dataLengthHigh;    /* High 16 bits of number of data bytes if
                                   CAP_LARGE_READX; else MUST BE ZERO */
    uint16_t reserved2[4];      /* Reserved (must be 0) */
    uint16_t byteCount;         /* Count of data bytes; ignored if
                                   CAP_LARGE_READX */
    uint8_t pad[];

    /* Data immediately follows */
}  __attribute__((__packed__))  READ_ANDX_RESPONSE_HEADER,
                               *PREAD_ANDX_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;           /* File handle */
    uint32_t offset;        /* Offset in file to begin read */
    uint16_t maxCount;      /* Max number of bytes to return */
    uint16_t minCount;      /* 0 = non-blocking named pipe read */
    uint32_t maxCountHigh;  /* High 16 bits of MaxCount if CAP_LARGE_READX;
                               else MUST BE ZERO */
    uint16_t remaining;     /* Reserved for obsolescent requests */
    uint32_t offsetHigh;    /* Upper 32 bits of offset
                               (only if wordCount is 12) */
    uint16_t byteCount;     /* Count of data bytes = 0 */
}  __attribute__((__packed__))  READ_REQUEST_HEADER, *PREAD_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t remaining;         /* Reserved -- must be -1 */
    uint16_t dataCompactionMode;
    uint16_t reserved;          /* Reserved (must be 0) */
    uint16_t dataLength;        /* Number of data bytes (min = 0) */
    uint16_t dataOffset;        /* Offset (from header start) to data */
    uint16_t dataLengthHigh;    /* High 16 bits of number of data bytes if
                                   CAP_LARGE_READX; else MUST BE ZERO */
    uint16_t reserved2[4];      /* Reserved (must be 0) */
    uint16_t byteCount;         /* Count of data bytes; ignored if
                                   CAP_LARGE_READX */
    uint8_t pad[];

    /* Data immediately follows */
}  __attribute__((__packed__))  READ_RESPONSE_HEADER, *PREAD_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;               /* File handle */
    uint32_t offset;            /* Offset in file to begin write */
    uint32_t reserved;          /* Must be 0 */
    uint16_t writeMode;         /* Write mode bits:
                                        0 - write through */
    uint16_t remaining;         /* Bytes remaining to satisfy request */
    uint16_t dataLengthHigh;    /* High 16 bits of data length if
                                   CAP_LARGE_WRITEX; else MUST BE ZERO */
    uint16_t dataLength;        /* Number of data bytes in buffer (>=0) */
    uint16_t dataOffset;        /* Offset to data bytes */
    uint16_t byteCount;         /* Count of data bytes; ignored if
                                   CAP_LARGE_WRITEX */
    uint8_t pad[];              /* Pad to SHORT or LONG */

    /* Data immediately follows */
}  __attribute__((__packed__))  WRITE_ANDX_REQUEST_HEADER_WC_12,
                               *PWRITE_ANDX_REQUEST_HEADER_WC_12;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;               /* File handle */
    uint32_t offset;            /* Offset in file to begin write */
    uint32_t reserved;          /* Must be 0 */
    uint16_t writeMode;         /* Write mode bits:
                                        0 - write through */
    uint16_t remaining;         /* Bytes remaining to satisfy request */
    uint16_t dataLengthHigh;    /* High 16 bits of data length if
                                   CAP_LARGE_WRITEX; else MUST BE ZERO */
    uint16_t dataLength;        /* Number of data bytes in buffer (>=0) */
    uint16_t dataOffset;        /* Offset to data bytes */
    uint32_t offsetHigh;        /* Upper 32 bits of offset (only present if
                                   WordCount = 14) */
    uint16_t byteCount;         /* Count of data bytes; ignored if
                                   CAP_LARGE_WRITEX */
    uint8_t pad[];              /* Pad to SHORT or LONG */

    /* Data immediately follows */
}  __attribute__((__packed__))  WRITE_ANDX_REQUEST_HEADER_WC_14,
                               *PWRITE_ANDX_REQUEST_HEADER_WC_14;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t count;             /* Number of bytes written */
    uint16_t remaining;         /* Reserved */
    uint16_t countHigh;         /* High 16? bits of data length if
                                   CAP_LARGE_WRITEX */
    uint16_t reserved;
    uint16_t byteCount;         /* Count of data bytes = 0 */
}  __attribute__((__packed__))  WRITE_ANDX_RESPONSE_HEADER,
                               *PWRITE_ANDX_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t fid;               /* File handle */
    uint16_t count;
    uint32_t offset;            /* Offset in file to begin write */
    uint16_t remaining;         /* Bytes remaining to satisfy request */

    uint16_t byteCount;         /* Count of data bytes */
    uint8_t  bufferFormat;
    uint16_t dataLength;        /* Number of data bytes in buffer (>=0) */

    /* Data immediately follows */
}  __attribute__((__packed__))  WRITE_REQUEST_HEADER, *PWRITE_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t count;             /* Number of bytes written */
    uint16_t byteCount;         /* Count of data bytes = 0 */
}  __attribute__((__packed__))  WRITE_RESPONSE_HEADER, *PWRITE_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT   usFileAttributes;
    ULONG    ulLastWriteTime;
    USHORT   reserved[5];
    USHORT   usByteCount;

    /* UCHAR ucBufferFormat; */
    /* PWSTR pwszPath */

} __attribute__((__packed__)) SET_INFO_REQUEST_HEADER,
                             *PSET_INFO_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT byteCount;         /* Count of data bytes = 0 */
}  __attribute__((__packed__))  SET_INFO_RESPONSE_HEADER,
                               *PSET_INFO_RESPONSE_HEADER;
typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT   usFid;
    SMB_DATE creationDate;
    SMB_TIME creationTime;
    SMB_DATE lastAccessDate;
    SMB_TIME lastAccessTime;
    SMB_DATE lastWriteDate;
    SMB_TIME lastWriteTime;
    USHORT   usByteCount;

} __attribute__((__packed__)) SET_INFO2_REQUEST_HEADER,
                             *PSET_INFO2_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT byteCount;         /* Count of data bytes = 0 */
}  __attribute__((__packed__))  SET_INFO2_RESPONSE_HEADER,
                               *PSET_INFO2_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT usFid;
    USHORT usByteCount;

} __attribute__((__packed__)) QUERY_INFO2_REQUEST_HEADER,
                             *PQUERY_INFO2_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    SMB_DATE creationDate;
    SMB_TIME creationTime;
    SMB_DATE lastAccessDate;
    SMB_TIME lastAccessTime;
    SMB_DATE lastWriteDate;
    SMB_TIME lastWriteTime;
    ULONG    ulFileDataSize;
    ULONG    ulFileAllocationSize;
    USHORT   usFileAttributes;
    USHORT   usByteCount;
}  __attribute__((__packed__))  QUERY_INFO2_RESPONSE_HEADER,
                               *PQUERY_INFO2_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t echoCount;
    uint16_t byteCount; /* Count of data bytes */

     /* Data immediately follows */
}  __attribute__((__packed__)) ECHO_REQUEST_HEADER, *PECHO_REQUEST_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t sequenceNumber;
    uint16_t byteCount; /* Count of data bytes */

     /* Data immediately follows */
}  __attribute__((__packed__)) ECHO_RESPONSE_HEADER, *PECHO_RESPONSE_HEADER;

typedef struct
{
    uint16_t byteCount;
} __attribute__((__packed__)) LOGOFF_REQUEST_HEADER, *PLOGOFF_REQUEST_HEADER;

typedef struct
{
    uint16_t byteCount;
} __attribute__((__packed__)) LOGOFF_RESPONSE_HEADER, *PLOGOFF_RESPONSE_HEADER;

typedef struct
{
    uint16_t fid;
    uint32_t ulLastWriteTime;
    uint16_t byteCount;

} __attribute__((__packed__)) CLOSE_REQUEST_HEADER, *PCLOSE_REQUEST_HEADER;

typedef struct
{
    uint16_t byteCount;
} __attribute__((__packed__)) CLOSE_RESPONSE_HEADER, *PCLOSE_RESPONSE_HEADER;

typedef struct _TRANS2_FILE_SMB_STANDARD_INFORMATION {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    ULONG FileSize;
    ULONG AllocationSize;
    USHORT Attributes;
    // ULONG EaSize;    Listed in the SNIA ref but not seen from WinXP
} __attribute__((__packed__)) TRANS2_FILE_SMB_STANDARD_INFORMATION,
                             *PTRANS2_FILE_SMB_STANDARD_INFORMATION;

typedef struct _TRANS2_FILE_SMB_INFO_STANDARD_SET {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
} __attribute__((__packed__)) TRANS2_FILE_SMB_INFO_STANDARD_SET,
                             *PTRANS2_FILE_SMB_INFO_STANDARD_SET;

typedef struct _TRANS2_FILE_BASIC_INFORMATION {
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    FILE_ATTRIBUTES FileAttributes;
    ULONG  unknown;
} __attribute__((__packed__)) TRANS2_FILE_BASIC_INFORMATION,
                             *PTRANS2_FILE_BASIC_INFORMATION;

typedef struct _TRANS2_FILE_STANDARD_INFORMATION {
    LONG64 AllocationSize;
    LONG64 EndOfFile;
    ULONG  NumberOfLinks;
    BOOLEAN bDeletePending;
    BOOLEAN bDirectory;
    USHORT  pad;
} __attribute__((__packed__)) TRANS2_FILE_STANDARD_INFORMATION,
                             *PTRANS2_FILE_STANDARD_INFORMATION;

typedef struct _TRANS2_FILE_ALL_INFORMATION {
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     unknown;
    LONG64    AllocationSize;
    LONG64    EndOfFile;
    ULONG     NumberOfLinks;
    BOOLEAN   bDeletePending;
    BOOLEAN   bDirectory;
    USHORT    pad;
    ULONG     EaSize;
    ULONG     FileNameLength;
    WCHAR     FileName[];
} __attribute__((__packed__)) TRANS2_FILE_ALL_INFORMATION,
                             *PTRANS2_FILE_ALL_INFORMATION;

typedef struct _TRANS2_FILE_NAME_INFORMATION
{
    ULONG     ulFileNameLength;
    WCHAR     FileName[];
} __attribute__((__packed__)) TRANS2_FILE_NAME_INFORMATION,
                             *PTRANS2_FILE_NAME_INFORMATION;

typedef struct _TRANS2_FILE_INTERNAL_INFORMATION {
    LONG64 IndexNumber;
} __attribute__((__packed__)) TRANS2_FILE_INTERNAL_INFORMATION,
                             *PTRANS2_FILE_INTERNAL_INFORMATION;

typedef struct _TRANS2_FILE_ATTRIBUTE_TAG_INFORMATION {
    ULONG FileAttributes;
    ULONG ReparseTag;
} __attribute__((__packed__)) TRANS2_FILE_ATTRIBUTE_TAG_INFORMATION,
                             *PTRANS2_ATTRIBUTE_TAG_INFORMATION;

typedef struct _TRANS2_FILE_FS_CONTROL_INFORMATION {
    LONG64          FreeSpaceStartFiltering;
    LONG64          FreeSpaceThreshold;
    LONG64          FreeSpaceStopFiltering;
    LONG64          DefaultQuotaThreshold;
    LONG64          DefaultQuotaLimit;
    FILE_VC_FLAGS   FileSystemControlFlags;
    ULONG           pad;
} __attribute__((__packed__)) TRANS2_FILE_FS_CONTROL_INFORMATION,
                             *PTRANS2_FILE_FS_CONTROL_INFORMATION;

typedef struct _TRANS2_FILE_NETWORK_OPEN_INFORMATION {
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    AllocationSize;
    LONG64    EndOfFile;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     pad;
} __attribute__((__packed__)) TRANS2_FILE_NETWORK_OPEN_INFORMATION,
                             *PTRANS2_FILE_NETWORK_OPEN_INFORMATION;

typedef struct {
    LONG64 EndOfFile;
} __attribute__((__packed__)) TRANS2_FILE_END_OF_FILE_INFORMATION,
                             *PTRANS2_FILE_END_OF_FILE_INFORMATION;

typedef struct {
    BOOLEAN bFileIsDeleted;
} __attribute__((__packed__)) TRANS2_FILE_DISPOSITION_INFORMATION,
                             *PTRANS2_FILE_DISPOSITION_INFORMATION;

typedef struct
{
    UCHAR  ucFlags;
    UCHAR  ucEaNameLength;
    USHORT usEaDataLength;
    UCHAR  ucName[1];

} __attribute__((__packed__))  TRANS2_FILE_EA_LIST_ENTRY,
                               PTRANS2_FILE_EA_LIST_ENTRY;
typedef struct
{
    ULONG ulEaListSize;
    TRANS2_FILE_EA_LIST_ENTRY pEaList[1];

} __attribute__((__packed__)) TRANS2_FILE_EA_LIST_INFORMATION,
                              *PTRANS2_FILE_EA_LIST_INFORMATION;

typedef struct _TRANS2_FILE_ACCESS_INFORMATION {
    ULONG AccessMask;
} __attribute__((__packed__)) TRANS2_FILE_ACCESS_INFORMATION,
                             *PTRANS2_FILE_ACCCESS_INFORMATION;

typedef struct
{
    UCHAR  ucReplaceIfExists;
    UCHAR  ucReserved[3];
    ULONG  ulRootDir;
    ULONG  ulFileNameLength;

} __attribute__((__packed__)) SMB_FILE_RENAME_INFO_HEADER,
                             *PSMB_FILE_RENAME_INFO_HEADER;

typedef struct _SMB_NOTIFY_INFO_HEADER
{
    ULONG     ulNextEntryOffset;
    ULONG     ulAction;
    ULONG     ulFileNameLength;
    WCHAR     wszFileName[1];

} __attribute__((__packed__)) SMB_NOTIFY_INFO_HEADER, *PSMB_NOTIFY_INFO_HEADER;

typedef struct _SMB_NT_TRANSACT_CREATE_REQUEST_HEADER
{
    ULONG       ulFlags;
    ULONG       ulRootDirectoryFid;
    ACCESS_MASK ulDesiredAccess;
    ULONG64     ullAllocationSize;
    ULONG       ulExtFileAttributes;
    ULONG       ulShareAccess;
    ULONG       ulCreateDisposition;
    ULONG       ulCreateOptions;
    ULONG       ulSecurityDescLength;
    ULONG       ulEALength;
    ULONG       ulNameLength;
    ULONG       ulImpersonationLevel;
    UCHAR       ucSecurityFlags;

    // WCHAR       pwszName[ulNameLength]; not null terminated

} __attribute__((__packed__))  SMB_NT_TRANSACT_CREATE_REQUEST_HEADER,
                             *PSMB_NT_TRANSACT_CREATE_REQUEST_HEADER;

typedef struct _SMB_NT_TRANSACT_CREATE_RESPONSE_HEADER
{
    UCHAR   ucOplockLevel;
    UCHAR   ucReserved;
    USHORT  usFid;
    ULONG   ulCreateAction;
    ULONG   ulEaErrorOffset;
    LONG64  llCreationTime;
    LONG64  llLastAccessTime;
    LONG64  llLastWriteTime;
    LONG64  llChangeTime;
    ULONG   ulExtFileAttributes;
    ULONG64 ullAllocationSize;
    ULONG64 ullEndOfFile;
    USHORT  usFileType;
    USHORT  usDeviceState;
    BOOLEAN bDirectory;
    UCHAR   ucReserved2[3];

} __attribute__((__packed__))  SMB_NT_TRANSACT_CREATE_RESPONSE_HEADER,
                             *PSMB_NT_TRANSACT_CREATE_RESPONSE_HEADER;

typedef struct _SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    UCHAR     ShortNameLength;
    UCHAR     Reserved;
    WCHAR     ShortName[12];
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER,
                             *PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER;

typedef struct _SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    UCHAR     ShortNameLength;
    UCHAR     Reserved;
    WCHAR     ShortName[12];
    USHORT    Reserved2;
    LONG64    FileId;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER,
                             *PSMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER;

typedef struct _SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    ULONG     Reserved;
    LONG64    FileId;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER,
                             *PSMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER;

typedef struct _SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER,
                             *PSMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER;

typedef struct _SMB_FIND_FILE_NAMES_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    ULONG     FileNameLength;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_NAMES_INFO_HEADER,
                             *PSMB_FIND_FILE_NAMES_INFO_HEADER;

typedef struct _SMB_FIND_FILE_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB_FIND_FILE_DIRECTORY_INFO_HEADER,
                             *PSMB_FIND_FILE_DIRECTORY_INFO_HEADER;

typedef struct _FIND_CLOSE2_REQUEST_HEADER {

    USHORT sid;
    USHORT usByteCount;

} __attribute__((__packed__)) FIND_CLOSE2_REQUEST_HEADER,
                             *PFIND_CLOSE2_REQUEST_HEADER;

typedef struct _FIND_CLOSE2_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) FIND_CLOSE2_RESPONSE_HEADER,
                             *PFIND_CLOSE2_RESPONSE_HEADER;

typedef struct _DELETE_DIRECTORY_REQUEST_HEADER {

    USHORT usByteCount;
    UCHAR  ucBufferFormat;

    /* PWSTR pwszDirectoryPath */

} __attribute__((__packed__)) DELETE_DIRECTORY_REQUEST_HEADER,
                             *PDELETE_DIRECTORY_REQUEST_HEADER;

typedef struct _DELETE_DIRECTORY_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) DELETE_DIRECTORY_RESPONSE_HEADER,
                             *PDELETE_DIRECTORY_RESPONSE_HEADER;

typedef struct _SMB_DELETE_REQUEST_HEADER
{
    SMB_FILE_ATTRIBUTES usSearchAttributes;
    USHORT ByteCount;

    /* UCHAR ucBufferFormat; */
    /* PWSTR pwszPath */

} __attribute__((__packed__)) SMB_DELETE_REQUEST_HEADER,
                             *PSMB_DELETE_REQUEST_HEADER;

typedef struct _SMB_DELETE_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) SMB_DELETE_RESPONSE_HEADER,
                             *PSMB_DELETE_RESPONSE_HEADER;

typedef struct _SMB_CREATE_DIRECTORY_REQUEST_HEADER
{
    USHORT ByteCount;

    /* UCHAR ucBufferFormat; */
    /* PWSTR pwszPath */

} __attribute__((__packed__)) SMB_CREATE_DIRECTORY_REQUEST_HEADER,
                             *PSMB_CREATE_DIRECTORY_REQUEST_HEADER;

typedef struct _SMB_CREATE_DIRECTORY_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) SMB_CREATE_DIRECTORY_RESPONSE_HEADER,
                             *PSMB_CREATE_DIRECTORY_RESPONSE_HEADER;

typedef struct _SMB_CHECK_DIRECTORY_REQUEST_HEADER {

    USHORT usByteCount;

    /* UCHAR ucBufferFormat; */
    /* PWSTR pwszDirectoryPath */

} __attribute__((__packed__)) SMB_CHECK_DIRECTORY_REQUEST_HEADER,
                             *PSMB_CHECK_DIRECTORY_REQUEST_HEADER;

typedef struct _SMB_CHECK_DIRECTORY_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) SMB_CHECK_DIRECTORY_RESPONSE_HEADER,
                             *PSMB_CHECK_DIRECTORY_RESPONSE_HEADER;


typedef struct _FLUSH_REQUEST_HEADER
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT usFid;
    USHORT usByteCount;

} __attribute__((__packed__)) FLUSH_REQUEST_HEADER, *PFLUSH_REQUEST_HEADER;

typedef struct _FLUSH_RESPONSE_HEADER {

    USHORT usByteCount;

} __attribute__((__packed__)) FLUSH_RESPONSE_HEADER, *PFLUSH_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t byteCount;

} __attribute__((__packed__)) ERROR_RESPONSE_HEADER, *PERROR_RESPONSE_HEADER;

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT         usFid;
    LWIO_LOCK_TYPE  ucLockType;
    UCHAR          ucOplockLevel;
    ULONG          ulTimeout;
    USHORT         usNumUnlocks;
    USHORT         usNumLocks;
    USHORT         usByteCount;

    /* LOCKING_ANDX_RANGE unlocks[]; */
    /* LOCKING_ANDX_RANGE locks[];   */
} __attribute__((__packed__)) SMB_LOCKING_ANDX_REQUEST_HEADER,
                             *PSMB_LOCKING_ANDX_REQUEST_HEADER;

typedef struct _SMB_LOCKING_ANDX_RESPONSE_HEADER
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT usByteCount;

} __attribute__((__packed__)) SMB_LOCKING_ANDX_RESPONSE_HEADER,
                             *PSMB_LOCKING_ANDX_RESPONSE_HEADER;

typedef struct _SMB_RENAME_REQUEST_HEADER
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    SMB_FILE_ATTRIBUTES usSearchAttributes;
    USHORT usByteCount;

} __attribute__((__packed__)) SMB_RENAME_REQUEST_HEADER,
                             *PSMB_RENAME_REQUEST_HEADER;

typedef struct _SMB_RENAME_RESPONSE_HEADER
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    USHORT usByteCount;

} __attribute__((__packed__)) SMB_RENAME_RESPONSE_HEADER,
                             *PSMB_RENAME_RESPONSE_HEADER;

#define SMB_NT_RENAME_SET_LINK_INFO    0x0103
#define SMB_NT_RENAME_RENAME_FILE      0x0104
#define SMB_NT_RENAME_MOVE_FILE        0x0105

typedef struct _SMB_NT_RENAME_REQUEST_HEADER
{
    SMB_FILE_ATTRIBUTES usSearchAttributes;
    USHORT usInfoLevel;
    ULONG ulClusterCount;
    USHORT usByteCount;
} __attribute__((__packed__)) SMB_NT_RENAME_REQUEST_HEADER,
                             *PSMB_NT_RENAME_REQUEST_HEADER;

typedef struct _SMB_NT_RENAME_RESPONSE_HEADER
{
    USHORT usByteCount;
    UCHAR  ucBuffer[1];
} __attribute__((__packed__)) SMB_NT_RENAME_RESPONSE_HEADER,
                             *PSMB_NT_RENAME_RESPONSE_HEADER;

typedef struct _SMB_NT_TRANS_QUERY_SECURITY_DESC_REQUEST_HEADER
{
    USHORT usFid;
    USHORT reserved;
    SECURITY_INFORMATION securityInformation;
} __attribute__((__packed__)) SMB_NT_TRANS_QUERY_SECURITY_DESC_REQUEST_HEADER,
                             *PSMB_NT_TRANS_QUERY_SECURITY_DESC_REQUEST_HEADER;

typedef enum
{
    ERROR_SMB,
    ERROR_NTSTATUS
} LWIO_ERROR_TYPE;

typedef struct
{
    LWIO_ERROR_TYPE type;   /* Error type: SMB or NTSTATUS */
    union
    {
        SMB_ERROR smb;     /* In error state, system error, if any  */
        NTSTATUS wire;     /* In error state, wire error, if any */
    };
} LWIO_ERROR_BUNDLE;

typedef enum
{
    SMB_RESOURCE_STATE_VALID = 0,

    /* Another thread is in setup; sleep */
    SMB_RESOURCE_STATE_INITIALIZING,

    /* @todo: determine if some resources need to be torn down completely
       (ACKed) before they can be recreated */

    /* Another thread is tearing, or has
       torn, down; create another resource.
       Resources are always removed from
       containers before this state is set.
     */
    SMB_RESOURCE_STATE_TEARDOWN,

    /* Another thread encountered an error which
       invalidated this resource; depending on
       the error code, find or recreate the
       resource or abort (negative cache entry). */
    SMB_RESOURCE_STATE_INVALID

} SMB_RESOURCE_STATE;

typedef enum
{
    SMB_SECURITY_MODE_SHARE = 0,
    SMB_SECURITY_MODE_USER
} SMB_SECURITY_MODE;

typedef struct _LWIO_PACKET_ALLOCATOR *PLWIO_PACKET_ALLOCATOR;

typedef struct _SMB2_FILE_BASIC_INFORMATION {
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    FILE_ATTRIBUTES FileAttributes;
    ULONG  unknown;
} __attribute__((__packed__)) SMB2_FILE_BASIC_INFORMATION,
                            *PSMB2_FILE_BASIC_INFORMATION;

NTSTATUS
MarshallNegotiateRequest(
    uint8_t       *pBuffer,
    uint32_t       bufferLen,
    uint32_t      *pBufferUsed,
    const uchar8_t *pszDialects[],
    uint32_t       dialectCount
    );

NTSTATUS
UnmarshallNegotiateRequest(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,        /* ByteCount found by caller */
    uchar8_t       *pszDialects[],
    uint32_t       *pDialectCount
    );

NTSTATUS
UnmarshallNegotiateResponse(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,
    NEGOTIATE_RESPONSE_HEADER **ppHeader,
    uint8_t       **ppGUID,
    uint8_t       **ppSecurityBlob,
    uint32_t       *blobLen
    );

NTSTATUS
MarshallSessionSetupRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uint8_t   *pSecurityBlob,
    uint16_t         blobLen,
    const wchar16_t *pwszNativeOS,
    const wchar16_t *pwszNativeLanMan,
    const wchar16_t *pwszWorkgroup
    );

NTSTATUS
MarshallSessionSetupResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uint8_t   *pSecurityBlob,
    uint16_t         blobLen,
    const wchar16_t *pwszNativeOS,
    const wchar16_t *pwszNativeLanMan,
    const wchar16_t *pwszNativeDomain
    );

NTSTATUS
UnmarshallSessionSetupResponse_WC_4(
    const uint8_t   *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    SESSION_SETUP_RESPONSE_HEADER_WC_4 **ppHeader,
    uint8_t        **ppSecurityBlob,
    wchar16_t      **ppwszNativeOS,
    wchar16_t      **ppwszNativeLanMan,
    wchar16_t      **ppwszNativeDomain
    );

NTSTATUS
UnmarshallSessionSetupRequest_WC_12(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    SESSION_SETUP_REQUEST_HEADER_WC_12 **ppHeader,
    uint8_t      **ppSecurityBlob,
    wchar16_t    **ppwszNativeOS,
    wchar16_t    **ppwszNativeLanMan,
    wchar16_t    **ppwszNativeDomain
    );

NTSTATUS
UnmarshallSessionSetupRequest_WC_13(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    SESSION_SETUP_REQUEST_HEADER_WC_13 **ppHeader,
    PLW_MAP_SECURITY_NTLM_LOGON_INFO     pSrvUserParams,
    wchar16_t    **ppwszNativeOS,
    wchar16_t    **ppwszNativeLanMan
    );

NTSTATUS
MarshallTreeConnectRequestData(
    OUT PBYTE pBuffer,
    IN ULONG bufferLen,
    IN uint8_t messageAlignment,
    OUT PULONG pBufferUsed,
    IN PCWSTR pwszPath,
    IN PCSTR pszService
    );

NTSTATUS
UnmarshallTreeConnectRequest(
    const PBYTE pBuffer,
    ULONG  ulBytesAvailable,
    ULONG  ulOffset,
    PTREE_CONNECT_REQUEST_HEADER* ppHeader,
    PBYTE* ppPassword,
    PWSTR* ppwszPath,
    PBYTE* ppszService
    );

NTSTATUS
MarshallTreeConnectResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferAvailable,
    uint32_t         bufferUsed,
    uint16_t        *pBufferUsed,
    const uchar8_t  *pszService,
    const wchar16_t *pwszNativeFileSystem
    );

NTSTATUS
UnmarshallTreeConnectExtResponse(
    const uint8_t    *pBuffer,
    uint32_t          bufferLen,
    uint8_t           messageAlignment,
    TREE_CONNECT_EXT_RESPONSE_HEADER **ppHeader
    );

NTSTATUS
WireUnmarshallCreateFileRequest(
    PBYTE  pParams,
    ULONG  ulBytesAvailable,
    ULONG  ulBytesUsed,
    PCREATE_REQUEST_HEADER* ppHeader,
    PWSTR* ppwszFilename
    );

NTSTATUS
WireMarshallCreateRequestData(
    OUT PBYTE pBuffer,
    IN ULONG bufferLen,
    IN uint8_t messageAlignment,
    OUT PULONG pBufferUsed,
    IN PCWSTR pwszPath
    );

NTSTATUS
WireUnmarshallSMBResponseCreate(
    IN PBYTE pBuffer,
    IN ULONG bufferLen,
    OUT PCREATE_RESPONSE_HEADER* ppHeader
    );

NTSTATUS
WireUnmarshallOpenRequest(
    PBYTE  pParams,
    ULONG  ulBytesAvailable,
    ULONG  ulBytesUsed,
    POPEN_REQUEST_HEADER* ppHeader,
    PWSTR* ppwszFilename
    );

NTSTATUS
WireMarshallTransactionRequestData(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    );

NTSTATUS
WireUnmarshallTransactionRequest(
    const PBYTE                  pBuffer,
    ULONG                        ulNumBytesAvailable,
    ULONG                        ulOffset,
    PTRANSACTION_REQUEST_HEADER* ppHeader,
    PUSHORT*                     ppSetup,
    PUSHORT*                     ppByteCount,
    PWSTR*                       ppwszName,
    PBYTE*                       ppParameters,
    PBYTE*                       ppData
    );

NTSTATUS
WireMarshallTransactionSecondaryRequestData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    );

NTSTATUS
WireUnmarshallTransactionSecondaryRequest(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PTRANSACTION_SECONDARY_REQUEST_HEADER* ppHeader,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    PBYTE*      ppData,
    USHORT      dataLen
    );

NTSTATUS
WireMarshallTransactionSecondaryResponseData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint16_t *pSetup,
    uint8_t   setupLen,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    );

NTSTATUS
WireUnmarshallTransactionSecondaryResponse(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PTRANSACTION_SECONDARY_RESPONSE_HEADER* ppHeader,
    PUSHORT*    ppSetup,
    PUSHORT*    ppByteCount,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    PBYTE*      ppData,
    USHORT      dataLen
    );

NTSTATUS
WireMarshallTransaction2Response(
    PBYTE       pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PUSHORT     pSetup,
    BYTE        setupCount,
    PBYTE       pParams,
    USHORT      usParamLength,
    PBYTE       pData,
    USHORT      usDataLen,
    PUSHORT     pusDataOffset,
    PUSHORT     pusParameterOffset,
    PUSHORT     pusNumPackageBytesUsed
    );

NTSTATUS
WireUnmarshallNtTransactionRequest(
    const PBYTE                     pBuffer,
    ULONG                           ulNumBytesAvailable,
    ULONG                           ulOffset,
    PNT_TRANSACTION_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppSetup,
    PUSHORT*                        ppByteCount,
    PBYTE*                          ppParameters,
    PBYTE*                          ppData
    );

NTSTATUS
WireMarshallNtTransactionResponse(
    PBYTE   pBuffer,
    ULONG   ulNumBytesAvailable,
    ULONG   ulOffset,
    PUSHORT pSetup,
    UCHAR   ucSetupCount,
    PBYTE   pParams,
    ULONG   ulParamLength,
    PBYTE   pData,
    ULONG   ulDataLen,
    PULONG  pulDataOffset,
    PULONG  pulParameterOffset,
    PULONG  pulNumPackageBytesUsed
    );

NTSTATUS
WireMarshallNtTransactionRequest(
    PBYTE   pBuffer,
    ULONG   ulNumBytesAvailable,
    ULONG   ulOffset,
    USHORT  usFunction,
    PUSHORT pSetup,
    UCHAR   ucSetupCount,
    PBYTE   pParams,
    ULONG   ulParamLength,
    PBYTE   pData,
    ULONG   ulDataLen,
    PULONG  pulDataOffset,
    PULONG  pulParameterOffset,
    PULONG  pulNumPackageBytesUsed
    );

NTSTATUS
WireUnmarshallWriteAndXRequest_WC_12(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulBytesUsed,
    PWRITE_ANDX_REQUEST_HEADER_WC_12* ppHeader,
    PBYTE*                 ppData
    );

NTSTATUS
WireUnmarshallWriteAndXRequest_WC_14(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulBytesUsed,
    PWRITE_ANDX_REQUEST_HEADER_WC_14* ppHeader,
    PBYTE*                 ppData
    );

NTSTATUS
MarshallWriteRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    uint16_t        *pDataOffset,
    uint8_t         *pWriteBuffer,
    uint16_t        wWriteLen
    );

NTSTATUS
WireUnmarshallReadAndXRequest_WC_10(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulBytesUsed,
    PREAD_ANDX_REQUEST_HEADER_WC_10* ppHeader
    );

NTSTATUS
WireUnmarshallReadAndXRequest_WC_12(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulBytesUsed,
    PREAD_ANDX_REQUEST_HEADER_WC_12* ppHeader
    );

NTSTATUS
WireMarshallReadResponseData(
    PBYTE  pDataBuffer,
    ULONG  ulBytesAvailable,
    ULONG  alignment,
    PVOID  pBuffer,
    ULONG  ulBytesToWrite,
    PULONG pulPackageByteCount
    );

NTSTATUS
WireMarshallReadResponseDataEx(
    PBYTE   pDataBuffer,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PVOID   pBuffer,
    ULONG   ulBytesToWrite,
    PULONG  pulDataOffset,
    PULONG  pulPackageByteCount
    );

NTSTATUS
MarshallReadRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed
    );

NTSTATUS
WireUnmarshallWriteRequest(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulBytesUsed,
    PWRITE_REQUEST_HEADER* ppHeader,
    PBYTE*                 ppData
    );

NTSTATUS
WireUnmarshallEchoRequest(
    const PBYTE           pBuffer,
    ULONG                 ulBufferLen,
    PECHO_REQUEST_HEADER* ppHeader,
    PBYTE*                ppEchoBlob
    );

NTSTATUS
WireMarshallEchoResponseData(
    const PBYTE pBuffer,
    ULONG       ulBufferLen,
    PBYTE       pEchoBlob,
    USHORT      usEchoBlobLength,
    PUSHORT     pusPackageByteCount
    );

NTSTATUS
WireUnmarshallCloseRequest(
    const PBYTE            pBuffer,
    ULONG                  ulBytesAvailable,
    ULONG                  ulBytesUsed,
    PCLOSE_REQUEST_HEADER* ppHeader
    );

NTSTATUS
WireUnmarshallFindClose2Request(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PUSHORT                pusSearchId
    );

NTSTATUS
WireMarshallFindClose2Response(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PUSHORT pusBytesUsed,
    PFIND_CLOSE2_RESPONSE_HEADER* ppResponseHeader
    );

NTSTATUS
WireUnmarshallDirectoryDeleteRequest(
    const PBYTE                       pParams,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PDELETE_DIRECTORY_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                            ppwszDirectoryPath
    );

NTSTATUS
WireMarshallDeleteDirectoryResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PDELETE_DIRECTORY_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallDeleteRequest(
    PBYTE                       pParams,
    ULONG                       ulBytesAvailable,
    ULONG                       ulOffset,
    PSMB_DELETE_REQUEST_HEADER* ppHeader,
    PWSTR*                      ppwszPath
    );

NTSTATUS
WireMarshallDeleteResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_DELETE_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallCreateDirectoryRequest(
    PBYTE                       pParams,
    ULONG                       ulBytesAvailable,
    ULONG                       ulOffset,
    PSMB_CREATE_DIRECTORY_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                      ppwszPath
    );

NTSTATUS
WireMarshallCreateDirectoryResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_CREATE_DIRECTORY_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallCheckDirectoryRequest(
    const PBYTE                       pParams,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PSMB_CHECK_DIRECTORY_REQUEST_HEADER*  ppRequestHeader,
    PWSTR*                            ppwszDirectoryPath
    );

NTSTATUS
WireMarshallCheckDirectoryResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_CHECK_DIRECTORY_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallLockingAndXRequest(
    PBYTE                             pParams,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PSMB_LOCKING_ANDX_REQUEST_HEADER* ppRequestHeader,
    PLOCKING_ANDX_RANGE*              ppUnlockRange,
    PLOCKING_ANDX_RANGE_LARGE_FILE*   ppUnlockRangeLarge,
    PLOCKING_ANDX_RANGE*              ppLockRange,
    PLOCKING_ANDX_RANGE_LARGE_FILE*   ppLockRangeLarge
    );

NTSTATUS
WireMarshallLockingAndXResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_LOCKING_ANDX_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallRenameRequest(
    PBYTE                       pParams,
    ULONG                       ulBytesAvailable,
    ULONG                       ulOffset,
    PSMB_RENAME_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                      ppwszOldName,
    PWSTR*                      ppwszNewName
    );

NTSTATUS
WireMarshallRenameResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_RENAME_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallNtRenameRequest(
    PBYTE                          pParams,
    ULONG                          ulBytesAvailable,
    ULONG                          ulOffset,
    PSMB_NT_RENAME_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                         ppwszOldName,
    PWSTR*                         ppwszNewName
    );

NTSTATUS
WireMarshallNtRenameResponse(
    PBYTE                           pParams,
    ULONG                           ulBytesAvailable,
    ULONG                           ulOffset,
    PSMB_NT_RENAME_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT                         pusPackageBytesUsed
    );

NTSTATUS
WireUnmarshallFlushRequest(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulOffset,
    PFLUSH_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
WireMarshallFlushResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PFLUSH_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    );

NTSTATUS
WireMarshallErrorResponse(
    PBYTE                   pParams,
    ULONG                   ulBytesAvailable,
    ULONG                   ulOffset,
    PERROR_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT                 pusParamBytesUsed
    );

BOOLEAN
SMBIsAndXCommand(
    uint8_t command
    );

NTSTATUS
SMBPacketCreateAllocator(
    IN ULONG ulNumMaxPackets,
    OUT PLWIO_PACKET_ALLOCATOR* ppPacketAllocator
    );

NTSTATUS
SMBPacketAllocate(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT PSMB_PACKET* ppPacket
    );

VOID
SMBPacketRelease(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN OUT PSMB_PACKET pPacket
    );

NTSTATUS
SMBPacketBufferAllocate(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN size_t len,
    OUT uint8_t** ppBuffer,
    OUT size_t* pAllocatedLen
    );

VOID
SMBPacketBufferFree(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT uint8_t* pBuffer,
    IN size_t bufferLen
    );

VOID
SMBPacketResetBuffer(
    PSMB_PACKET pPacket
    );

VOID
SMBPacketFreeAllocator(
    IN OUT PLWIO_PACKET_ALLOCATOR pPacketAllocator
    );

VOID
SMBPacketHTOLSmbHeader(
    IN OUT SMB_HEADER* pHeader
    );

/* @todo: support AndX */
/* @todo: support signing */
NTSTATUS
SMBPacketMarshallHeader(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    uint8_t     command,
    uint32_t    error,
    uint32_t    isResponse,
    uint16_t    tid,
    uint32_t    pid,
    uint16_t    uid,
    uint16_t    mid,
    BOOLEAN     bCommandAllowsSignature,
    PSMB_PACKET pPacket
    );

NTSTATUS
SMBPacketMarshallFooter(
    PSMB_PACKET pPacket
    );

BOOLEAN
SMBPacketIsSigned(
    PSMB_PACKET pPacket
    );

BOOLEAN
SMB2PacketIsSigned(
    PSMB_PACKET pPacket
    );

NTSTATUS
SMBPacketVerifySignature(
    PSMB_PACKET pPacket,
    ULONG       ulExpectedSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
SMB2PacketVerifySignature(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
SMBPacketDecodeHeader(
    IN OUT PSMB_PACKET pPacket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN OPTIONAL PBYTE pSessionKey,
    IN DWORD dwSessionKeyLength
    );

NTSTATUS
SMBPacketSign(
    PSMB_PACKET pPacket,
    ULONG       ulSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
SMB2PacketSign(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
SMBPacketUpdateAndXOffset(
    PSMB_PACKET pPacket
    );

NTSTATUS
SMBPacketAppendUnicodeString(
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN PCWSTR pwszString
    );

NTSTATUS
SMBPacketAppendString(
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN PCSTR pszString
    );

/**
 * @brief Marshal setup portion of trans2 request
 *
 * Marshals the setup portion of a trans2 request and
 * returns pointers to areas that need to be filled in.
 *
 * @param[in, out] pSmbHeader a pointer to the SMB header in the packet.
 * This is used as a reference point for alignment.  Upon return, the
 * WordCount field will be set to the correct value for the request.
 * @param[in,out] ppCursor the data cursor.  Upon call it must point
 * to the parameters portion of the SMB packet.  Upon return
 * it will point to the beginning of the trans2 parameters
 * block.
 * @param[in,out] pulRemainingSpace remaining space in the buffer.  Upon
 * call it should contain the number of bytes of space available
 * after the cursor.  Upon return it will be updated to reflect
 * how much space is left.
 * @param[in] pusSetupWords a pointer to the setup words
 * @param[in] usSetupWordCount the number of setup words
 * @param[out] ppRequestHeader the static portion of the request
 * @param[out] ppByteCount the location of the byte count field
 *
 * @return an NT status code
 * @retval STATUS_SUCCESS success
 * @retval STATUS_BUFFER_TOO_SMALL the remaining space was exceeded during marshaling
 */
NTSTATUS
WireMarshalTrans2RequestSetup(
    IN OUT PSMB_HEADER               pSmbHeader,
    IN OUT PBYTE*                    ppCursor,
    IN OUT PULONG                    pulRemainingSpace,
    IN PUSHORT                       pusSetupWords,
    IN USHORT                        usSetupWordCount,
    OUT PTRANSACTION_REQUEST_HEADER* ppRequestHeader,
    OUT PBYTE*                       ppByteCount
    );

/**
 * @brief Unmarshal setup portion of trans2 response
 *
 * Unmarshals the setup portion of a trans2 request and
 * returns pointers to the segments of interest within
 * the packet.
 *
 * @param[in] pSmbHeader a pointer to the SMB header in the packet.
 * This is used as a reference point for alignment.
 * @param[in,out] ppCursor the data cursor.  Upon call it must point
 * to the parameters portion of the SMB packet.  Upon return
 * it will point exactly past the end of the trans2 reponse.
 * @param[in,out] pulRemainingSpace remaining space in the buffer.  Upon
 * call it should contain the number of bytes of space available
 * after the cursor.  Upon return it will be updated to reflect
 * how much space is left.
 * @param[optional,out] ppResponseHeader a pointer to the static portion of the
 * response header.
 * @param[optional,out] ppusSetupWords a pointer to the setup words in the response
 * @param[optional,out] pusSetupWordCount the number of setup words in the response
 * @param[optional,out] pusByteCount the number of total bytes in the response
 * after the setup words
 * @param[optional,out] ppParamterBlock a pointer to the start of the parameter block
 * @param[optional,out] ppDataBlock a pointer to the start of the data block
 *
 * @return an NT status code
 * @retval STATUS_SUCCESS success
 * @retval STATUS_BUFFER_TOO_SMALL the remaining space was exceeded during marshaling
 */
NTSTATUS
WireUnmarshalTrans2ReplySetup(
    IN PSMB_HEADER                                        pSmbHeader,
    IN OUT PBYTE*                                         ppCursor,
    IN OUT PULONG                                         pulRemainingSpace,
    OPTIONAL OUT PTRANSACTION_SECONDARY_RESPONSE_HEADER*  ppResponseHeader,
    OPTIONAL OUT PUSHORT                                  pusTotalParameterCount,
    OPTIONAL OUT PUSHORT                                  pusTotalDataCount,
    OPTIONAL OUT PUSHORT*                                 ppusSetupWords,
    OPTIONAL OUT PUSHORT                                  pusSetupWordCount,
    OPTIONAL OUT PUSHORT                                  pusByteCount,
    OPTIONAL OUT PBYTE*                                   ppParameterBlock,
    OPTIONAL OUT PUSHORT                                  pusParameterCount,
    OPTIONAL OUT PBYTE*                                   ppDataBlock,
    OPTIONAL OUT PUSHORT                                  pusDataCount
    );

NTSTATUS
WireUnmarshalSetInfoRequest(
    PBYTE                  pBuffer,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PSET_INFO_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                      ppwszPath
    );

NTSTATUS
WireUnmarshalSetInfo2Request(
    PBYTE                  pBuffer,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PSET_INFO2_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
WireUnmarshalQueryInfo2Request(
    PBYTE                        pBuffer,
    ULONG                        ulBytesAvailable,
    ULONG                        ulOffset,
    PQUERY_INFO2_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
WireGetCurrentNTTime(
    PLONG64 pllCurTime
    );

NTSTATUS
WireNTTimeToTimeSpec(
    LONG64 llCurTime,
    struct timespec* pTimeSpec
    );

NTSTATUS
WireNTTimeToSMBDateTime(
    LONG64    llNTTime,
    PSMB_DATE pSmbDate,
    PSMB_TIME pSmbTime
    );

NTSTATUS
WireSMBDateTimeToNTTime(
    PSMB_DATE pSmbDate,
    PSMB_TIME pSmbTime,
    PLONG64   pllNTTime
    );

NTSTATUS
WireNTTimeToSMBUTime(
    LONG64 llNTTime,
    PULONG pulSmbUTime
    );

NTSTATUS
WireSMBUTimetoNTTime(
    ULONG   ulSmbUTime,
    BOOLEAN bAdjustToLocalTimeZone,
    PLONG64 pllNTTime
    );

NTSTATUS
WireSMBUTimeToTimeZone(
    time_t Time,
    PSHORT psTimezone
    );

#endif /* __SMBWIRE_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
