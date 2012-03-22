/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
/*
**
**  NAME
**
**      twr_np.c
**
**  FACILITY:
**
**      Protocol Tower Services for the internet address family
**
**  ABSTRACT:
**
**      The protocol tower service provides routines that:
**      
**      o  convert a socket address to the lower floors of 
**         a protocol tower 
**
**      o  convert a protocol tower to a socket address
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <twrp.h>	/* Private tower services */
#include <npnaf.h>
       
/*
 *  Include the Internet specific socket address
 */
#if HAVE_SYS_UN_H
#include <sys/un.h>
#else
#include <un.h>
#endif
#include <sys/param.h>
#include <netdb.h>
#include <ctype.h>

/*
**++
**
**  ROUTINE NAME:       twr_np_lower_flrs_from_sa
**
**  SCOPE:              PUBLIC - declared in twr.idl
**
**  DESCRIPTION:
**
**  Creates the canonical representation of an internet protocol tower's 
**  lower floors from a sockaddr. The canonical form can be transmitted 
**  on the wire, or included in a DNS tower.
**
**  INPUTS:
**
**      trans_prot      Integer value specifying the transport layer 
**                      protocol for the Internet address family. 
**                      For address family RPC_C_NAF_ID_NP specify:
**
**                         RPC_C_NETWORK_PROTOCOL_ID_NP for udp
**
**      sa              Internet-specific socket address data 
**                      structure.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      lower_flrs      Returns the lower tower floors in a twr_t 
**                      structure (includes the floor count in the 
**                      "tower_octet_string" member).
**
**      status          A value indicating the return status of the routine:
**                          twr_s_ok
**                          twr_s_unknown_sa
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/


PUBLIC void twr_np_lower_flrs_from_sa 
(
    sockaddr_p_t      sa,
    twr_p_t           *lower_flrs,
    unsigned32        *status
)
{
    unsigned8   protocol_id[TWR_C_NUM_NP_LOWER_FLRS];
    unsigned16  id_size = TWR_C_TOWER_PROT_ID_SIZE,
                floor_count,
                related_data_size[TWR_C_NUM_NP_LOWER_FLRS],
                twr_rep_16;
    unsigned32  count,
                twr_t_length;
    byte_p_t    related_data_ptr[TWR_C_NUM_NP_LOWER_FLRS],
                tmp_tower;
    char hostname[MAXHOSTNAMELEN], *p;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    RPC_DBG_GPRINTF(("(twr_np_lower_flrs_from_sa) called\n"));

    if (sa->family == RPC_C_NAF_ID_UXD)
    {
        protocol_id[0] = TWR_C_FLR_PROT_ID_NP;
        protocol_id[1] = TWR_C_FLR_PROT_ID_NB;
    }
    else
    {
        *status = twr_s_unknown_sa;
        return;
    }

    /*
     * Since we now know the socket address we're dealing with, 
     * collect the sizes of each field to allocate memory, and 
     * remember the pointers to the fields so we can copy the 
     * data once we have allocated the tower string.
     */
    floor_count = TWR_C_NUM_NP_LOWER_FLRS;
            
    /*
     * related_data[0] contains the pipe name (eg. \PIPE\lsass)
     */
    related_data_ptr[0] = (unsigned char*) ((struct sockaddr_un *)sa)->sun_path;
    if (strncmp((char*) related_data_ptr[0], (char*) RPC_C_NP_DIR"/", RPC_C_NP_DIR_LEN + 1) == 0)
    {
        /* Path is relative, but do not chop off first /. */
        related_data_ptr[0] += RPC_C_NP_DIR_LEN;
    }
    related_data_size[0] = strlen((char*) related_data_ptr[0]) + 1;

    /*
     * related_data[1] contains the NetBIOS name (eg. \\MYSERVER)
     */
    hostname[0] = '\\';
    hostname[1] = '\\';
    gethostname(&hostname[2], MAXHOSTNAMELEN - 3);
    hostname[MAXHOSTNAMELEN - 1] = '\0';
    for (p = &hostname[2]; *p != '\0'; p++)
        *p = toupper((int)*p);
    related_data_size[1] = strlen(hostname) + 1;
    related_data_ptr[1] = (unsigned char*) hostname;

    /*  
     * Calculate the length of the tower floors.
     */
     
    twr_t_length = TWR_C_TOWER_FLR_COUNT_SIZE;  /* to store floor count */

    for ( count = 0; count < floor_count; count++ )
    {
        twr_t_length += TWR_C_FLR_OVERHEAD;
        twr_t_length += related_data_size[count];
    }

    /* 
     * Next allocate space for the tower structure
     */
    RPC_MEM_ALLOC (
        *lower_flrs,
        twr_p_t,
        sizeof (twr_t) + (twr_t_length - 1),
        RPC_C_MEM_TOWER,
        RPC_C_MEM_WAITOK );

    /* 
     * Copy the length of the tower octet string into the tower structure
     */
    (*lower_flrs)->tower_length = twr_t_length;
    
    /* 
     * Copy the floor information into the tower octet string 
     */

    /*
     * Use a temporary for the octet string since we need
     * to increment the pointer.
     */
    tmp_tower = (*lower_flrs)->tower_octet_string;

    /* 
     * Copy the number of floors into the tower octet string 
     */
    twr_rep_16 = floor_count;
    RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
    memcpy ((char *)tmp_tower, (char *)&twr_rep_16, 
            TWR_C_TOWER_FLR_COUNT_SIZE);

    tmp_tower += TWR_C_TOWER_FLR_COUNT_SIZE;

    /* 
     * Convert the protocol identifier size to its proper
     * representation for use in the following loop.
     */
    RPC_RESOLVE_ENDIAN_INT16 (id_size);

    for ( count = 0; count < floor_count; count++ )
    {
        /* 
         * Copy the length of the protocol identifier field into 
         * tower octet string.  (Converted before the loop.)
         */
        memcpy ((char *)tmp_tower, (char *)&id_size, 
                TWR_C_TOWER_FLR_LHS_COUNT_SIZE);

        tmp_tower += TWR_C_TOWER_FLR_LHS_COUNT_SIZE;


        /* 
         * Copy the protocol identifier into tower octet string
         * (1 byte so no need to convert endian representation).
         */
        memcpy ((char *)tmp_tower, (char *)&(protocol_id[count]),
                TWR_C_TOWER_PROT_ID_SIZE);

        tmp_tower += TWR_C_TOWER_PROT_ID_SIZE;

        /* 
         * Copy the length of the address data field into 
         * tower octet string.
         */
        twr_rep_16 = related_data_size[count];
        RPC_RESOLVE_ENDIAN_INT16 (twr_rep_16);
        memcpy ((char *)tmp_tower, (char *)&twr_rep_16,
                TWR_C_TOWER_FLR_RHS_COUNT_SIZE);
 
        tmp_tower += TWR_C_TOWER_FLR_RHS_COUNT_SIZE;

        /* 
         * If there is addressing data, copy the address data field into 
         * tower octet string
         */
        if (related_data_size[count])
        {
            memcpy ((char *)tmp_tower, (char *)related_data_ptr[count], 
                    related_data_size[count]);

            /* Convert slashes */
            for (p = (char*) tmp_tower; *p != '\0'; p++) {
                if (*p == '/')
                    *p = '\\';
            }

            /*
             * Set up for the next floor.
             */
            tmp_tower += related_data_size[count];
        }
    }

    *status = twr_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       twr_np_lower_flrs_to_sa 
**
**  SCOPE:              PUBLIC - declared in twr.idl
**
**  DESCRIPTION:
**
**  Creates an internet sockaddr from the canonical representation of an 
**  internet protocol tower's lower floors.
**
**  INPUTS:
**
**      tower_octet_string
**                      The protocol tower to convert to a sockaddr.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      sa            Returns a pointer to the created sockaddr structure.
**      
**      sa_len          Returns the length, in bytes, of the returned 
**                      "sa" argument.
**
**      status          A value indicating the return status of the routine:
**                          twr_s_ok
**                          twr_s_unknown_tower
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void twr_np_lower_flrs_to_sa 
(
    byte_p_t          tower_octet_string, 
    sockaddr_p_t      *sa,
    unsigned32        *sa_len,
    unsigned32        *status
)
{
    unsigned8   id;
    byte_p_t    tower;
    unsigned16  count,
                floor_count,
                id_size,
                addr_size;
    unsigned32  length;
    char       *p;

    CODING_ERROR (status);
    RPC_VERIFY_INIT ();

    id_size = 0;

    /* 
     * Make sure we have a pointer to some data structure.
     */
    if ( !(tower = tower_octet_string)) 
    {
        *status = twr_s_unknown_tower;
        return;
    }

    RPC_DBG_GPRINTF(("(twr_np_lower_flrs_to_sa) called\n"));

    /* 
     * Get the tower floor count 
     */
    memcpy ((char *)&floor_count, (char *)tower, TWR_C_TOWER_FLR_COUNT_SIZE);
    RPC_RESOLVE_ENDIAN_INT16 (floor_count);

    tower += TWR_C_TOWER_FLR_COUNT_SIZE;

    /*
     * Skip over the (application's) upper floors while we look for the 
     * beginning of the np-specific lower floors.
     */
    for ( count = 0; count < floor_count; count++ )
    {
        /*
         * Get the length of this floor's protocol id field (don't advance
         * the pointer).
         */
        memcpy ((char *)&id_size, (char *)tower, 
                TWR_C_TOWER_FLR_LHS_COUNT_SIZE);
        RPC_RESOLVE_ENDIAN_INT16 (id_size);

        /*
         * Get the protocol id (don't advance the pointer).
         * Expect one byte; no need to convert.
         */
        memcpy ((char *)&id, (char *)(tower + TWR_C_TOWER_FLR_LHS_COUNT_SIZE),
                TWR_C_TOWER_PROT_ID_SIZE);

        /*
         * See if we support the protocol id.
         */
        if ( (id_size == TWR_C_TOWER_PROT_ID_SIZE) && 
             (id == TWR_C_FLR_PROT_ID_NP))
        {
            /*
             * Indicate we found the beginning of the np floors.
             */
            *status = twr_s_ok;

            break;
        }
        else
        {
            /*
             * Skip this floor.  Get the address size in order
             * to know how much to skip.
             */
            memcpy ((char *)&addr_size, 
                    (char *)(tower + TWR_C_TOWER_FLR_LHS_COUNT_SIZE +
                            id_size), TWR_C_TOWER_FLR_RHS_COUNT_SIZE);
            RPC_RESOLVE_ENDIAN_INT16 (addr_size);

            tower += TWR_C_TOWER_FLR_LHS_COUNT_SIZE + id_size +
                     TWR_C_TOWER_FLR_RHS_COUNT_SIZE + addr_size;

            /*
             * For now, assume we don't find the floors we're looking for.
             */
            *status = twr_s_unknown_tower;
        }
    }

    if (*status != twr_s_ok)
    {
        return;
    }

    /*
     * Skip the floor's protocol id field length and protocol id
     * (now move the pointer).  We already know it's
     * TWR_C_FLR_PROT_ID_NP.
     */
    tower += (TWR_C_TOWER_FLR_LHS_COUNT_SIZE + id_size);


    /* 
     * Allocate space for sockaddr
     */
    length = sizeof(struct sockaddr_un);
                    
    RPC_MEM_ALLOC (
        *sa,
        sockaddr_p_t,
        length,
        RPC_C_MEM_SOCKADDR,
        RPC_C_MEM_WAITOK );
    
    *sa_len = length;
                    
    /*
     * make sure unused bytes are null 
     */
    memset ((char *) *sa, 0, length);
                    
    /* 
     * define this as an internet family socket
     */
    ((struct sockaddr_un *)(*sa))->sun_family = RPC_C_NAF_ID_UXD;
                    
    /* 
     * Get the length of pipe name
     */
    memcpy ((char *)&addr_size, (char *)tower, RPC_C_TOWER_FLR_RHS_COUNT_SIZE);
    RPC_RESOLVE_ENDIAN_INT16 (addr_size);
    tower += RPC_C_TOWER_FLR_RHS_COUNT_SIZE;

    /*
     * Copy the pipe name to the sockaddr
     */
    tower[addr_size - 1] = '\0';
    addr_size += RPC_C_NP_DIR_LEN + 1;
    if ((size_t)addr_size + 1 > sizeof(((struct sockaddr_un *)(*sa))->sun_path))
    {
        *status = rpc_s_no_memory;
        RPC_MEM_FREE (*sa, RPC_C_MEM_SOCKADDR);
        return;
    }
    snprintf(((struct sockaddr_un *)(*sa))->sun_path, length, "%s/%s",
             RPC_C_NP_DIR, tower);
    for (p = ((struct sockaddr_un *)(*sa))->sun_path; *p != '\0'; p++)
    {
        if (*p == '\\')
            *p = '/';
    }

    *status = twr_s_ok;
 
    /* For now, disregard the NetBIOS address. */
}
