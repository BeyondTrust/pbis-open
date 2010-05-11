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
 *        archive.h
 *
 * Abstract:
 *
 *        Archive file API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ARCHIVE_H__
#define __LWMSG_ARCHIVE_H__

#include <lwmsg/assoc.h>
#include <lwmsg/protocol.h>

#include <sys/types.h>

/**
 * @file archive.h
 * @brief Persistence API
 */

/**
 * @defgroup archive Persistent archives
 * @ingroup public
 * @brief Serialize messages to permanent storage
 *
 */

/*@{*/

/**
 * @brief Archive handle
 *
 * A handle used to open, read, write, and
 * close archive files.
 */
typedef struct LWMsgArchive LWMsgArchive;

/**
 * @brief Archive file open disposition
 *
 * Disposition used to open an archive file.
 */
typedef enum LWMsgArchiveDisposition
{
    /**
     * Open archive for reading
     * @hideinitializer
     */
    LWMSG_ARCHIVE_READ = 0x1,
    /**
     * Open archive for writing
     * @hideinitializer
     */
    LWMSG_ARCHIVE_WRITE = 0x2,
    /**
     * Read/write protocol schema to/from file
     */
    LWMSG_ARCHIVE_SCHEMA = 0x4
} LWMsgArchiveDisposition;

/**
 * @brief Create new archive handle
 *
 * Creates a new handle for reading and writing archive files
 * for the specified protocol.
 *
 * @param[in] context an optional context
 * @param[in] protocol a protocol specifying the available messages
 * to be read and written
 * @param[out] archive the archive handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgArchive** archive
    );

/**
 * @brief Set file name and parameters
 *
 * Sets the archive file name, file disposition (read or write), and
 * UNIX permissions (when writing) for the given archive handle.
 *
 * @param[in,out] archive the archive handle
 * @param[in] filename the path to the archive file
 * @param[in] disp the file disposition
 * @param[in] mode the UNIX permissions used when writing a file that does not yet exist
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_set_file(
    LWMsgArchive* archive,
    const char* filename,
    LWMsgArchiveDisposition disp,
    mode_t mode
    );

/**
 * @brief Set file descriptor
 *
 * Sets the file descriptor and file disposition (read or write) for the given
 * archive handle.
 *
 * @param[in,out] archive the archive handle
 * @param[in] fd the file descriptor
 * @param[in] disp the file disposition
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_set_fd(
    LWMsgArchive* archive,
    int fd,
    LWMsgArchiveDisposition disp
    );

/**
 * @brief Set byte order
 *
 * Sets the preferred byte order for archive file creation.
 * This only has an effect when creating new archives.
 *
 * @param[in,out] archive the archive handle
 * @param[in] order the byte order
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_set_byte_order(
    LWMsgArchive* archive,
    LWMsgByteOrder order
    );

/**
 * @brief Open archive file
 *
 * Opens the underlying archive specified by
 * #lwmsg_archive_set_file() or #lwmsg_archive_set_fd().
 * Messages may then be read and written with #lwmsg_archive_read_message()
 * and #lwmsg_archive_write_message().
 * 
 * @param[in,out] archive the archive handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the underlying file was malformed}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_open(
    LWMsgArchive* archive
    );

/**
 * @brief Close archive file
 *
 * Closes the underlying archive specified by
 * #lwmsg_archive_set_file() or #lwmsg_archive_set_fd().
 * All written messages will be flushed.
 * 
 * @param[in,out] archive the archive handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_close(
    LWMsgArchive* archive
    );

/**
 * @brief Write message into archive
 *
 * Write a message into an open archive.
 * 
 * @param[in,out] archive the archive handle
 * @param[in] message the message to write
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, the provided message was malformed}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_write_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

/**
 * @brief Read next message from archive
 *
 * Reads the next message from an open archive.
 * 
 * @param[in,out] archive the archive handle
 * @param[out] message the read message
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, the underlying file was malformed}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_read_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

/**
 * @brief Destroy a message
 *
 * Frees all memory allocated for a message previously read
 * from the given archive.
 *
 * @param[in] archive the archive handle
 * @param[in,out] message the message to destroy
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, the message was malformed}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_archive_destroy_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    );

/**
 * @brief Delete archive handle
 *
 * Deletes the given archive handle.
 * 
 * @param[in,out] archive the archive handle
 */
void
lwmsg_archive_delete(
    LWMsgArchive* archive
    );

/**
 * @brief View archive as an association
 *
 * Returns a "view" of the given archive handle as an association.
 * The association may then be used as usual to "send" and "receive"
 * messages, although some features (session, nonblocking operation)
 * will not be available.  Using #lwmsg_assoc_delete() on the
 * returned pointer is equivalent to deleting the archive handle.
 *
 * @param[in] archive the archive handle
 * @return a view of the archive as an association
 */
LWMsgAssoc*
lwmsg_archive_as_assoc(
    LWMsgArchive* archive
    );

/*@}*/

#endif
