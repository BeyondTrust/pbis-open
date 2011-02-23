/* ex: set shiftwidth=4 expandtab: */
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
**      comutil.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Utility services.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */


/*
 *****************************************************************************
 *
 * ASCII to EBCDIC (and vice versa) conversion tables.
 *
 * These tables were snarfed from the V2 ndr_chars.c module
 * from the IDL stub runtime support.
 * INTERNAL for use by the rpc_util_strcvt() routine.
 *
 *****************************************************************************
 */
 
INTERNAL unsigned_char_t cvt_ascii_to_ebcdic[] = {
     0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F, /* 0x00 - 0x07 */
     0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, /* 0x08 - 0x0F */
     0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26, /* 0x10 - 0x17 */
     0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F, /* 0x18 - 0x1F */
     0x40, 0x4F, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D, /* 0x20 - 0x27 */
     0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61, /* 0x28 - 0x2F */
     0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, /* 0x30 - 0x37 */
     0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F, /* 0x38 - 0x3F */
     0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, /* 0x40 - 0x47 */
     0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, /* 0x48 - 0x4F */
     0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, /* 0x50 - 0x57 */
     0xE7, 0xE8, 0xE9, 0x4A, 0xE0, 0x5A, 0x5F, 0x6D, /* 0x58 - 0x5F */
     0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 0x60 - 0x67 */
     0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, /* 0x68 - 0x6F */
     0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, /* 0x70 - 0x77 */
     0xA7, 0xA8, 0xA9, 0xC0, 0xBB, 0xD0, 0xA1, 0x07, /* 0x78 - 0x7F */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0x80 - 0x87 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0x88 - 0x8F */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0x90 - 0x97 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0x98 - 0x9F */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xA0 - 0xA7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xA8 - 0xAF */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xB0 - 0xB7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xB8 - 0xBF */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xC0 - 0xC7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xC8 - 0xCF */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xD0 - 0xD7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xD8 - 0xDF */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xE0 - 0xE7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xE8 - 0xEF */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, /* 0xF0 - 0xF7 */
     0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFF  /* 0xF8 - 0xFF */
};

INTERNAL unsigned_char_t cvt_ebcdic_to_ascii[] = {
     0x00, 0x01, 0x02, 0x03, 0x1A, 0x09, 0x1A, 0x7F, /* 0x00 - 0x07 */
     0x1A, 0x1A, 0x1A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, /* 0x08 - 0x0F */
     0x10, 0x11, 0x12, 0x13, 0x1A, 0x1A, 0x08, 0x1A, /* 0x10 - 0x17 */
     0x18, 0x19, 0x1A, 0x1A, 0x1C, 0x1D, 0x1E, 0x1F, /* 0x18 - 0x1F */
     0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x0A, 0x17, 0x1B, /* 0x20 - 0x27 */
     0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x05, 0x06, 0x07, /* 0x28 - 0x2F */
     0x1A, 0x1A, 0x16, 0x1A, 0x1A, 0x1A, 0x1A, 0x04, /* 0x30 - 0x37 */
     0x1A, 0x1A, 0x1A, 0x1A, 0x14, 0x15, 0x1A, 0x1A, /* 0x38 - 0x3F */
     0x20, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x40 - 0x47 */
     0x1A, 0x1A, 0x5B, 0x2E, 0x3C, 0x28, 0x2B, 0x21, /* 0x48 - 0x4F */
     0x26, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x50 - 0x57 */
     0x1A, 0x1A, 0x5D, 0x24, 0x2A, 0x29, 0x3B, 0x5E, /* 0x58 - 0x5F */
     0x2D, 0x2F, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x60 - 0x67 */
     0x1A, 0x1A, 0x1A, 0x2C, 0x25, 0x5F, 0x3E, 0x3F, /* 0x68 - 0x6F */
     0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x70 - 0x77 */
     0x1A, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22, /* 0x78 - 0x7F */
     0x1A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* 0x80 - 0x87 */
     0x68, 0x69, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x88 - 0x8F */
     0x1A, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, /* 0x90 - 0x97 */
     0x71, 0x72, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0x98 - 0x9F */
     0x1A, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, /* 0xA0 - 0xA7 */
     0x79, 0x7A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xA8 - 0xAF */
     0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xB0 - 0xB7 */
     0x1A, 0x1A, 0x1A, 0x7C, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xB8 - 0xBF */
     0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0xC0 - 0xC7 */
     0x48, 0x49, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xC8 - 0xCF */
     0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, /* 0xD0 - 0xD7 */
     0x51, 0x52, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xD8 - 0xDF */
     0x5C, 0x1A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, /* 0xE0 - 0xE7 */
     0x59, 0x5A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, /* 0xE8 - 0xEF */
     0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0xF0 - 0xF7 */
     0x38, 0x39, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0xFF  /* 0xF8 - 0xFF */
};

/*
**++
**
**  ROUTINE NAME:       rpc_string_free
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  This routine will free the memory allocated for a string data structure.
**  A NULL pointer will be returned.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      string          A pointer to the string pointer for the memory to
**                      be freed.
**
**  OUTPUTS:
**
**      status          A value indicating the result of the routine.
**
**          rpc_s_ok        The call was successful.
**          rpc_s_coding_error
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

PUBLIC void rpc_string_free 
(
    unsigned_char_p_t       *string,
    unsigned32              *status
)
{
    CODING_ERROR (status);
   
    if (*string != NULL)
    { 
        RPC_MEM_FREE (*string, RPC_C_MEM_STRING);
        *string = NULL;
    }

    *status = rpc_s_ok;
    return;
}

/* Reads a token from string into the token variable. The token starts at
 * string and ends when any character in deliminators is encountered.
 *
 * Zero length tokens will be returned if the first character in string is a
 * deliminator.
 *
 * If escape is not '\0', then it will escape itself and any deliminators. If
 * an escape character is followed by a non-special character (not a
 * deliminator and not the escape character), then the escape character is
 * stored in the string. Parsing is always stopped at a '\0', whether or not
 * it is escaped.
 *
 * The token variable may be null, in which case the function performs
 * normally, but the token is not returned.
 *
 * The return value is the number of raw characters (including escapes) that
 * occur before the first deliminator. The deliminator that terminated the
 * token can be found by looking at string[retvalue].
 */
PRIVATE size_t rpc__get_token(
    const unsigned char *string,
    unsigned_char_t	    escape,
    const char *            deliminators,
    unsigned_char_p_t       *token,
    unsigned32              *status
)
{
    size_t raw_size = 0, escaped_size = 0, read_pos = 0, write_pos = 0;
    unsigned_char_t is_deliminator[256];

    CODING_ERROR (status);
   
    if (string == NULL)
        string = (const unsigned char *)"";

    memset(is_deliminator, 0, sizeof(is_deliminator));
    for(raw_size = 0; deliminators[raw_size] != '\0'; raw_size++)
    {
        is_deliminator[(int) deliminators[raw_size]] = 1;
    }
    is_deliminator[0] = 1;

    //Figure out how long token should be before writing it
    escaped_size = 0;
    for(raw_size = 0; !is_deliminator[string[raw_size]]; raw_size++)
    {
        if(string[raw_size] == escape)
        {
            unsigned_char_t next_char = string[raw_size + 1];
            if((is_deliminator[next_char] && next_char != '\0') ||
                    next_char == escape)
            {
                //This character is an escape because it is the escape
                //character and the next character is special.
                raw_size++;
            }
        }
        escaped_size++;
    }

    if(token == NULL)
        return raw_size;

    RPC_MEM_ALLOC (
        *token,
        unsigned_char_p_t,
        escaped_size + 1,
        RPC_C_MEM_STRING,
        RPC_C_MEM_NOWAIT);
    if(*token == NULL)
    {
        *status = rpc_s_no_memory;
        return raw_size;
    }

    write_pos = 0;
    for(read_pos = 0; read_pos < raw_size && write_pos < escaped_size; read_pos++)
    {
        if(string[read_pos] == escape)
        {
            unsigned_char_t next_char = string[read_pos + 1];
            if((is_deliminator[next_char] && next_char != '\0') ||
                    next_char == escape)
            {
                //This character is an escape because it is the escape
                //character and the next character is special.
                read_pos++;
            }
        }
        token[0][write_pos++] = string[read_pos];
    }

    token[0][write_pos] = '\0';

    if(read_pos != raw_size || write_pos != escaped_size)
    {
        //Some other thread concurrently modified the string. It shouldn't do
        //that
        RPC_MEM_FREE (*token, RPC_C_MEM_STRING);
        *status = rpc_s_invalid_arg;
        *token = NULL;
        return raw_size;
    }

    *status = rpc_s_ok;
    return raw_size;
}


/*
**++
**
**  ROUTINE NAME:       rpc__strcspn
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Searches a given string for any of a set of given terminating
**  characters and returns a count of the number of non-terminating
**  characters in the string preceding the first occurrance of any
**  member of the terminating set.
**
**  The same signature and basically the same semantics as strcspn,
**  except that members of the terminator set will not terminate the
**  string if preceded by an escape (\) character.  Note that the escape
**  character cannot be used as a member of the terminator set. 
**
**  INPUTS:
**
**      string          A pointer to the string to be searched.
**
**      terms           A string of characters which are to be treated
**                      as terminators.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      A count of the number of character in the string preceding the
**      first occurrance of any member of the terminating set. The count
**      will be set to zero if no member of the terminating set is found
**      in the string, or the string pointer is NULL. If a member of the 
**      terminating set is escaped ('\') in "string", it will be skipped
**      over.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__strcspn 
(
    unsigned_char_p_t       string,
    char                    *term_set
)
{
    unsigned_char_p_t   ptr;
    unsigned_char_p_t   term_ptr;
    unsigned32          count;
    unsigned32          escaped;


    /*
     * make sure there's something to do before we start
     */
    if (string == NULL)
    {
        return (0);
    }

    /*
     * search for one of the given terminators
     */
    for (count = 1, escaped = false, ptr = string; *ptr != '\0'; count++, ptr++)
    {
        /*
         * Check to see if the current character is an escape character
         * and if so, skip over it to the next character, setting the flag. 
         */
        if (*ptr == '\\')
        {
            escaped = true;
            ptr++;
        }

        /*
         * make sure it's not the end of the line
         */
        if (*ptr == '\0')
        {
            break;
        }

        if (escaped == true) 
        {
            /*
             * Watch out for multiple '\' 
             */
            if (*ptr != '\\')
            {
                escaped = false;
            }
            ptr++;
            continue;
        }

        if (escaped == false) 
        {
            /*
             * search the terminator set for a match
             */
            for (term_ptr = (unsigned_char_p_t) term_set;
                 *term_ptr != '\0'; 
                 term_ptr++)
            {
                if (*ptr == *term_ptr)
                {
                    return (count);
                }
            }
        }
    }

    /*
     * if no terminator in the set was found...
     */
    return (0);
}
    
/*
**++
**
**  ROUTINE NAME:       rpc__strncpy
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Copies n characters from one string into another.
**  The same signature and basically the same semantics as strncpy,
**  except that the destination string is *always* null terminated.
**  For this reason care must be taken that the value n is always
**  one less than the available size for the target. Also, this routine
**  doesn't bother to return useless information like strncpy.
**
**  INPUTS:
**
**      dst_string      A pointer to the destination string.
**
**      src_string      A pointer to the source string.
**
**      max_len         The maximum number of characters to be copied
**                      from the source to the destination.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
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

PRIVATE void rpc__strncpy 
(
    unsigned_char_p_t       dst_string,
    unsigned_char_p_t       src_string,
    unsigned32              max_len
)
{
    /*
     * make sure there's something to do before we start
     */
    if (src_string == NULL || dst_string == NULL || max_len == 0)
    {
        if (dst_string != NULL) *dst_string = '\0';
        return;
    }

    /*
     * just use strncpy, but unlike strncpy, add in a null terminator
     */
    strncpy ((char *) dst_string, (char *) src_string, max_len);
    *(dst_string + max_len) = '\0';
}

/*
**++
**
**  ROUTINE NAME:       rpc__strsqz
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Compress white space (space and tab characters) out of a given
**  string. An escaped space or tab will be preserved in the result.
**  Spaces, tabs and escapes within quoted strings will be preserved
**  in the result. The opening quote character can be included in the
**  quote by preceding it with the same character.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      string          A pointer to the string to be compressed.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      A count of the number of character in the string after it has
**      been compressed. The count will be set to zero if the string
**      pointer is NULL.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__strsqz 
(
    unsigned_char_t         *string
)
{
    unsigned_char_p_t   ptr1, ptr2;
    unsigned32          count;


    /*
     * make sure there's something to do before we start
     */
    if (string == NULL)
    {
        return (0);
    }

    for (count = 0, ptr1 = ptr2 = string; *ptr1 != '\0'; ptr1++)
    {
        /*
         * immediately after an escape all characters are valid
         */
        if (*ptr1 == '\\')
        {
            /*
             * get the escape character itself
             */
            *(ptr2++) = *(ptr1++);
            count++;

            /*
             * if the next character is not the end of the line, get that too
             */
            if (*ptr1 != '\0')
            {
                *(ptr2++) = *ptr1;
                count++;
            }
        }
        else
        {
            /*
             * if we're not escaped, eliminate spaces and tabs
             */
            if (*ptr1 != ' ' && *ptr1 != '\t')        
            {
                *(ptr2++) = *ptr1;
                count++;
            }
        }
    }

    /*
     * terminate the destination and return its size
     */
    *ptr2 = '\0';
    return (count);
}



/*
**++
**
**  ROUTINE NAME:       rpc_stralloc
**
**  SCOPE:              PUBLIC - declared in rpcpvt.idl
**
**  DESCRIPTION:
**      
**  Make a copy of the input string into alloc'd storage.
**
**  INPUTS:             
**
**      string          A pointer to the string to be copied.
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      Pointer to alloc'd string.
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC unsigned_char_p_t rpc_stralloc (string)

unsigned_char_p_t       string;

{
    unsigned_char_p_t   cstring;
                        

    RPC_MEM_ALLOC (
        cstring,
        unsigned_char_p_t,
        strlen ((char *) string) + 1,
        RPC_C_MEM_STRING,
        RPC_C_MEM_WAITOK);

    strcpy ((char *) cstring, (char *) string);

    return (cstring);
}

/*
**++
**
**  ROUTINE NAME:       rpc__stralloc
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Old routine whose functionality has been replaced
**  by rpc_stralloc.  
**
**  INPUTS:             
**
**      string          A pointer to the string to be copied.
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      Pointer to alloc'd string.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned_char_p_t rpc__stralloc 
(
    unsigned_char_p_t       string
)
{

    return (rpc_stralloc (string));

}


/*
**++
**
**  ROUTINE NAME:       rpc_util_strcvt
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Routine to convert between ASCII and EBCDIC character representations.
**
**  INPUTS:
**
**	to_ascii	boolean flag indicating which conversion to make
**			a 'true' value indicates to convert from ebcdic to
**			ascii
**
**	len		length of string to be converted
**
**  INPUTS/OUTPUTS:     none
**
**      src		pointer to source string pointer
**	dst		pointer to destination string pointer
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:	none
**
**  SIDE EFFECTS:       none
**
**--
**/

PUBLIC void rpc_util_strcvt 
(
    boolean32		to_ascii,
    unsigned32		len,
    unsigned_char_p_t	src,
    unsigned_char_p_t	dst
)
{
    unsigned_char_p_t cvt_tbl;

    RPC_VERIFY_INIT ();

    cvt_tbl = to_ascii ? cvt_ebcdic_to_ascii : cvt_ascii_to_ebcdic;

    while ( len-- )
    {
    	*dst++ = cvt_tbl[*src++];
    }
}
