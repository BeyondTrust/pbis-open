/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
