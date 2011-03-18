/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        archive-private.h
 *
 * Abstract:
 *
 *        Archive file API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ARCHIVE_PRIVATE_H__
#define __LWMSG_ARCHIVE_PRIVATE_H__

#include <lwmsg/data.h>
#include <lwmsg/archive.h>
#include "assoc-private.h"

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>

#define ARCHIVE_PRIVATE(assoc) ((LWMsgArchive*) (assoc))

#define PACKED __attribute__((packed))

struct LWMsgArchive
{
    LWMsgAssoc base;
    int fd;
    char* file;
    off_t offset;
    LWMsgArchiveDisposition disp;
    mode_t mode;
    LWMsgByteOrder byte_order;
    LWMsgBool update_protocol;
    uint8_t version_major;
    uint8_t version_minor;
    LWMsgDataContext* data_context;
};

#define ARCHIVE_VERSION_FLAG_BIG_ENDIAN 0x1
#define ARCHIVE_VERSION_MAJOR 1
#define ARCHIVE_VERSION_MINOR 1
#define ARCHIVE_VERSION_MICRO 0

#define ARCHIVE_FORMAT_FLAG_SCHEMA 0x1

typedef struct ArchiveHeader
{
    uint8_t magic[4];
    uint8_t version_flags;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_micro;
    uint32_t format_flags;
    uint8_t protocol_id[16];
} PACKED ArchiveHeader;

typedef struct ArchiveMessage_v0
{
    uint32_t status;
    uint32_t cookie;
    int32_t tag;
    uint32_t size;
    uint8_t data[];
} PACKED ArchiveMessage_v0;

typedef struct ArchiveMessage
{
    uint32_t status;
    uint16_t flags;
    uint16_t cookie;
    int16_t tag;
    uint32_t size;
    uint8_t data[];
} PACKED ArchiveMessage;

LWMsgStatus
lwmsg_archive_write_header_fd(
    LWMsgArchive* archive
    );

LWMsgStatus
lwmsg_archive_write_message_fd(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

LWMsgStatus
lwmsg_archive_read_header_fd(
    LWMsgArchive* archive
    );

LWMsgStatus
lwmsg_archive_read_message_fd(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

#define ARCHIVE_RAISE_ERROR(_archive_, ...) BAIL_ON_ERROR(status = RAISE(&(_archive_)->base.context, __VA_ARGS__))

#endif
