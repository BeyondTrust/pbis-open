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
 *        marshal-private.h
 *
 * Abstract:
 *
 *        Marshalling API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_MARSHAL_PRIVATE_H__
#define __LWMSG_MARSHAL_PRIVATE_H__

#include <lwmsg/data.h>
#include "type-private.h"
#include "status-private.h"

struct LWMsgDataContext
{
    LWMsgErrorContext error;
    const LWMsgContext* context;
    LWMsgByteOrder byte_order;
};

typedef struct LWMsgMarshalState
{
    unsigned char* dominating_object;
} LWMsgMarshalState;

#define MAX_INTEGER_SIZE (16)

#define MARSHAL_RAISE_ERROR(hand, expr, ...) \
    BAIL_ON_ERROR(lwmsg_data_context_raise_error((hand), (expr), __VA_ARGS__))

typedef struct LWMsgUnmarshalState
{
    unsigned char* dominating_object;
} LWMsgUnmarshalState;

typedef LWMsgStatus (*LWMsgGraphVisitFunction) (
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    );

LWMsgStatus
lwmsg_data_visit_graph(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    );

LWMsgStatus
lwmsg_data_visit_graph_children(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    );

LWMsgStatus
lwmsg_data_extract_discrim_tag(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    intmax_t* tag
    );

LWMsgStatus
lwmsg_data_extract_length(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    size_t *length
    );

LWMsgStatus
lwmsg_data_extract_active_arm(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    LWMsgTypeIter* active_iter
    );

LWMsgStatus
lwmsg_data_verify_range(
    LWMsgErrorContext* error,
    LWMsgTypeIter* iter,
    void* object,
    size_t object_size
    );

LWMsgStatus
lwmsg_data_calculate_indirect_metrics(
    LWMsgTypeIter* iter,
    unsigned char* object,
    size_t* count,
    size_t* element_size
    );

LWMsgStatus
lwmsg_data_free_graph_internal(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    unsigned char* object
    );
    
#endif
