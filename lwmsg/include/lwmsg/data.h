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
 *        data.h
 *
 * Abstract:
 *
 *        Data model API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_DATA_H__
#define __LWMSG_DATA_H__

#include <lwmsg/status.h>
#include <lwmsg/type.h>
#include <lwmsg/buffer.h>
#include <lwmsg/context.h>

/**
 * @file data.h
 * @brief Data model API
 */

/**
 * @defgroup data Data model
 * @ingroup public
 * @brief Data marshaling and manipulation
 *
 * The LWMsg data model allows ordinary C data structures to
 * be converted to and from flat octet-stream representations.
 * Before data structures can be processed, a type specification
 * must be constructed which describes the layout and relationships
 * of the structure fields.  For details, see @ref types.
 *
 * The top level object passed to or returned by marshaller operations
 * must always be a pointer.  As a convenience, certain top-level type
 * specifications will automatically be promoted to pointers to accomodate
 * this restriction:
 * - Structures
 *  
 * All operations take a data context which controls tunable
 * parameters of the data model:
 *
 * <ul>
 * <li>Byte order of the octect representation</li>
 * </ul>
 *
 * In the event of an error, the data context can be queried for
 * a human-readable diagnostic message.  The data context may
 * also reference an LWMsg context.
 *
 */

/*@{*/

/**
 * @brief Data context
 * 
 * An opqaue data context which facilitates all
 * data model operations.
 */
typedef struct LWMsgDataContext LWMsgDataContext;

/**
 * @brief Create a data context
 *
 * Creates a new data context with an optional context.  Data operations
 * which return allocated memory (e.g. #lwmsg_data_unmarshal()) will use
 * the context's memory management functions.
 *
 * @param[in] context an optional context
 * @param[out] dcontext the created data context
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_context_new(
    const LWMsgContext* context,
    LWMsgDataContext** dcontext
    );

/**
 * @brief Delete a data context
 *
 * Deletes the specified data context.
 *
 * @param[in,out] context the data context
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
void
lwmsg_data_context_delete(
    LWMsgDataContext* context
    );

/**
 * @brief Set byte order
 *
 * Sets the byte order which will be used for atomic data elements
 * in subsequent marshal and unmarshal operations using the specified
 * data context.
 *
 * The default byte order is network byte order (big endian).  This is
 * the preferred order for data destined for long-term storage or
 * transmission over a network.
 *
 * @param[out] context the data context
 * @param[in] order the data order
 */
void
lwmsg_data_context_set_byte_order(
    LWMsgDataContext* context,
    LWMsgByteOrder order
    );

/**
 * @brief Get Byte order
 *
 * Gets the byte order which will be used for subsequent marshal and
 * unmarshal operations on the specified data context.
 *
 * @param[in] context the data context
 * @return the current byte order
 */
LWMsgByteOrder
lwmsg_data_context_get_byte_order(
    LWMsgDataContext* context
    );

/**
 * @brief Get context
 *
 * Gets the context which was given to #lwmsg_data_context_new()
 * when the specified data context was created.
 *
 * @param[in] context the data context
 * @return the context, or NULL if no context was given at creation time
 */
const LWMsgContext*
lwmsg_data_context_get_context(
    LWMsgDataContext* context
    );

/**
 * @brief Free in-memory data graph
 *
 * Recursively frees the data graph rooted at <tt>root</tt>
 * whose type is specified by <tt>type</tt>.  Each contiguous
 * memory object will be freed using the context given to
 * #lwmsg_data_context_new() when the specified data context
 * was created.
 *
 * The most common application of this function is to free
 * the entire data graph allocated by a prior unmarshal operation,
 * such as #lwmsg_data_unmarshal().
 *
 * Because the root of the data graph is always specified by
 * a generic pointer, <tt>type</tt> is subject to pointer
 * promotion.
 *
 * @param[in,out] context the data context
 * @param[in] type the type of the root node of the graph
 * @param[in,out] root the root of the graph
 */
LWMsgStatus
lwmsg_data_free_graph(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* root
    );

/**
 * @brief Destroy in-memory data graph
 *
 * Recursively destroys the data graph rooted at <tt>root</tt>
 * whose type is specified by <tt>type</tt>.  In contrast to
 * #lwmsg_data_free_graph(), the top-level structure is not freed.
 * Every child memory object will be freed using the context given
 * to #lwmsg_data_context_new() when the specified data context
 * was created.
 *
 * @param[in,out] context the data context
 * @param[in] type the type of the root node of the graph
 * @param[in,out] root the root of the graph
 */
LWMsgStatus
lwmsg_data_destroy_graph(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* root
    );

/**
 * @brief Free in-memory data graph (guaranteed success)
 *
 * Like #lwmsg_data_free_graph(), but guarantees success:
 *
 * - Does not require an #LWMsgDataContext to be allocated
 *   (which could itself fail).  Instead, an optional #LWMsgContext
 *   can be passed in to specify the memory manager.
 * - Does not return an error
 *
 * @warning This function has undefined behavior if the passed
 * data graph is malformed.
 *
 * @param[in] context an optional context
 * @param[in] type the type of the root node of the graph
 * @param[in,out] root the root of the graph
 */
void
lwmsg_data_free_graph_cleanup(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* root
    );

/**
 * @brief Destroy in-memory data graph (guaranteed success)
 *
 * Like #lwmsg_data_destroy_graph(), but guarantees success
 * in the same way as #lwmsg_data_free_graph_cleanup().
 *
 * @warning This function has undefined behavior if the passed
 * data graph is malformed.
 *
 * @param[in] context an optional context
 * @param[in] type the type of the root node of the graph
 * @param[in,out] root the root of the graph
 */
void
lwmsg_data_destroy_graph_cleanup(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* root
    );

/**
 * @brief Marshal a data structure
 *
 * Converts a data structure of the specified type to a flat, serialized form, storing
 * the result in the provided marshalling buffer.
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[in,out] buffer the marshalling buffer into which the result will be stored
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    LWMsgBuffer* buffer
    );

/**
 * @brief Marshal a data structure into a simple buffer
 *
 * Converts a data structure of the specified type to a flat, serialized form, storing
 * the result in the provided simple buffer.
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[out] buffer the buffer into which the result will be stored
 * @param[in] length the size of the buffer in bytes
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{EOF, the buffer was not large enough to hold the entire serialized representation}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal_flat(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void* buffer,
    size_t length
    );

/**
 * @brief Marshal a data structure while allocating a buffer
 *
 * Converts a data structure of the specified type to a flat, serialized form, automatically
 * allocating sufficient space for the result.  The buffer is allocated using the memory management
 * functions in context passed to #lwmsg_data_context_new().
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[out] buffer the allocated buffer containing the serialized representation
 * @param[out] length the length of the buffer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal_flat_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void** buffer,
    size_t* length
    );

/**
 * @brief Unmarshal a data structure
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in,out] buffer the marshalling buffer from which data will be read
 * @param[out] out the resulting unmarshalled object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_unmarshal(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    LWMsgBuffer* buffer,
    void** out
    );

/**
 * @brief Unmarshal a data structure into pre-allocated memory
 *
 * Converts a serialized data structure to its unmarshalled form without allocating memory for the
 * top-level structure -- instead, the caller must provide sufficient space for it.  Memory for
 * embedded pointers will still be allocated automatically as in #lwmsg_data_unmarshal().
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in,out] buffer the marshalling buffer from which data will be read
 * @param[out] object the memory object into which the unmarshalled data will be written
 * @param[in] size the size of <tt>object</tt> in bytes
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{BUFFER_TOO_SMALL, <tt>object</tt> was too small to contain the unmarshalled structure}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_unmarshal_into(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    LWMsgBuffer* buffer,
    void* object,
    size_t size
    );

/**
 * @brief Unmarshal a data structure from a simple buffer
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.  The serialized form is read from a simple buffer rather than a
 * full #LWMsgBuffer.
 *
 * @param[in,out] context the data context
 * @param[in] type the type specification which describes the type of the data
 * @param[in] buffer the simple buffer from which data will be read
 * @param[in] length the length of the buffer in bytes
 * @param[out] out the resulting unmarshalled object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_unmarshal_flat(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    const void* buffer,
    size_t length,
    void** out
    );

/**
 * @brief Print textual representation of a data graph to a string
 *
 * Prints a human-readable textual representation of a data graph,
 * automatically allocating and returning a C string containing the
 * result.  The string will be allocated using the context passed
 * to #lwmsg_data_context_new() when creating the data context.
 *
 * @param[in,out] context the data context
 * @param[in] type a type specification which describes the graph root
 * @param[in] root the root of the data graph
 * @param[out] result the resulting string
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_print_graph_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* root,
    char** result
    );

/**
 * @brief Allocate memory block
 *
 * Allocates a block of memory using the data context's memory manager.
 * This is the same memory manager used when reconstructing an object graph
 * during unmarshalling.
 *
 * @param[in] context the data context
 * @param[in] size the number of bytes to allocate
 * @param[out] object the allocated block
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_alloc_memory(
    LWMsgDataContext* context,
    size_t size,
    void** object
    );

/**
 * @brief Free memory block
 *
 * Frees a block of memory using the data context's memory manager.
 * This is the same memory manager used when reconstructing an object graph
 * during unmarshalling.
 *
 * @param[in] context the data context
 * @param[in,out] object the block to free
 */
void
lwmsg_data_free_memory(
    LWMsgDataContext* context,
    void* object
    );

/*@}*/

#endif
