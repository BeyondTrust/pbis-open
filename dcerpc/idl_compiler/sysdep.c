/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**
**  NAME
**
**      SYSDEP.C
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Operating system dependencies.
**
**  VERSION: DCE 1.0
**
*/


#include <nidl.h>

#ifdef vms
# include <types.h>
# include <stat.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif

#ifndef MAX_INCLUSION_DEPTH
# define MAX_INCLUSION_DEPTH 10
#endif

#ifndef HASPOPEN
static int temp_count = 0;
static char *temp_names[MAX_INCLUSION_DEPTH];

char *sysdep_save_temp
(
    char *old_name
)
{
#ifndef vms
        char new_name[64];
        char *new_name_ptr;
        sprintf(new_name,"IDL%02d.TMP", temp_count);
        new_name_ptr = temp_names[temp_count] = NEW_VEC (char, strlen(new_name) + 1);
        strcpy(temp_names[temp_count++], new_name);
        unlink(new_name);
        if(rename(old_name, new_name))
        {
                error(NIDL_RENAMEFAILED,old_name,new_name);
        }
        return(new_name_ptr);
#endif
}

void sysdep_cleanup_temp
()
{
#ifndef vms
        int i;
        char *name;

        for(i = 0; i < temp_count; i++)
        {
                name = temp_names[i];
                free(temp_names[i]);
                temp_names[i] = (char *) 0;
                if(name)
                {
                        unlink(temp_names[i]);
                }
        }
        temp_count = 0;
#endif
}

#endif


#ifdef MSDOS
/* Make a legal unix file name from a DOS one */
void msdos_fix_filename
(
    char *name
)
{
        while(*name)
        {
                if(*name == '\\')
                        *name = '/';
                name++;
        }
}

#endif


#if defined(IDL_USE_OUTPUT_LINE)
#undef fprintf

/*
** isidchar - tests whether a character is a valid identifier character:
**            alphanumeric or an underscore.
*/
#define isidchar(c) \
    (  (c) == '_' \
    || ((c) >= 'a' && (c) <= 'z') \
    || ((c) >= 'A' && (c) <= 'Z') \
    || ((c) >= '0' && (c) <= '9') )

/*
** Choose an arbitrary large limit for the longest line length.
** Sorry, but we have to have one.  Note that (LINE_BUFF_SIZE - MAX_C_LINE_LEN)
** is the largest size output string that is guaranteed to fit in the buffer.
*/
#define LINE_BUFF_SIZE 2048
#ifdef DUMPERS
#define MAX_C_LINE_LEN   80
#else
#define MAX_C_LINE_LEN  132
#endif
#define MAX_FORMAT_LEN  255
#define INDENT_SP_PER_LVL 2
#if defined(vax) && !defined(vms)
# include <varargs.h>
#else
# include <stdarg.h>
#endif
#include <stdio.h>

static char out_buffer[LINE_BUFF_SIZE]; /* Buffer for outputs */
static char new_format[MAX_FORMAT_LEN]; /* Buffer for modified format string */
static int buff_len = 0;                /* Length of current data in buffer */
static int indent_sp = 0;               /* Number of spaces to indent */
static FILE * previous_fid = NULL;      /* File id from previous call */


/*
**  f l u s h _ o u t p u t _ l i n e
**
**  Routine that must be called after all output_line calls to a file are
**  complete, to write any leftover data to the given file.
**  It can be called at other times if desired.
**
**  Implicit inputs:  out_buffer, buff_len, previous_fid
*/

void flush_output_line
(
    FILE * fid                  /* [in] File handle */
)

{
    /*
    ** If fid does not match fid from last call to output_line, this is a noop
    ** since output_line always calls this routine on a fid switch so that
    ** data isn't lost for a previous fid.
    */
    if (fid != previous_fid)
        return;

    /*
    ** Data always starts at out_buffer[0]; buff_len is length of data.
    */
    if (fid != NULL && buff_len > 0)
    {
        out_buffer[buff_len] = '\0';
        fprintf (fid, "%s", out_buffer);
        buff_len = 0;
    }
}


/*
**  o u t p u t _ l i n e
**
**  Replacement routine for fprintf() for use by the backend on platforms
**  whose C compiler has a limit of N characters per source line.  A call to
**  fprintf, or a series of calls to fprintf without a newline character, can
**  cause source lines greater than N characters.  This routine assures that
**  no source lines over N characters will be output by breaking long lines
**  up into more than one line when necessary.
**
**  Implicit inputs:  out_buffer, buff_len, previous_fid
*/

int output_line
(
    FILE * fid,                 /* [in] File handle */
    char *format,               /* [in] Format string */
    ...                         /* [in] 0-N format arguments */
)

{
    va_list args;
    char *buff, *obuff, *cp, *pcp;
    char temp;
    int i, j, len, new_len;

#if defined(vax) && !defined(vms)
    va_start (args);
#else
    va_start (args, format);
#endif

    /*
    ** If not the same fid as the last call, flush the output buffer for the
    ** previous fid if it has any data.  This makes us lose track of that fid,
    ** and a illegally long line becomes possible.
    */
    if (fid != previous_fid)
    {
        if (buff_len > 0)
            flush_output_line(previous_fid);
        previous_fid = fid;
    }

    /*
    ** When data is buffered across calls, it always begins at out_buffer[0].
    ** buff points to the next chunk of the data to be output.
    ** buff_len counts the remaining number of bytes to be output.
    */
    buff = obuff = out_buffer;

    /*
    ** Munge the format string for indentation.
    */
    new_len = 0;

    for (pcp = cp = format ; *cp ; cp++)
    {
        switch (*cp)
        {
        case '\n':
            /*
            ** Copy format string through newline [and insert indentation].
            */
            len = cp - pcp + 1;
            strncpy(&new_format[new_len], pcp, len);
            pcp = cp + 1;
            new_len += len;
            /*
            ** Lookahead: special case logic for certain next characters.
            */
            if (cp[1] == '\0' || cp[1] == '#')
                break;  /* Don't insert indentation */
            j = ((cp[1] == '}') ? indent_sp - INDENT_SP_PER_LVL : indent_sp);
            for (i = 0 ; i < j ; i++)
                new_format[new_len++] = ' ';
            break;

        case '{':
            indent_sp += INDENT_SP_PER_LVL;
            break;

        case '}':
            /*
            ** Reduce indent level in buf if '}' first char of format string.
            */
            if (cp == format && buff_len == indent_sp)
                buff_len -= INDENT_SP_PER_LVL;
            indent_sp -= INDENT_SP_PER_LVL;
            break;

        case '#':
            /*
            ** Remove indentation in buf if '#' first char of format string.
            */
            if (cp == format && buff_len == indent_sp)
                buff_len = 0;
            break;

        default:
            break;
        }
    }

    strcpy(&new_format[new_len], pcp);  /* Copy rest of format string */
    buff_len += vsprintf (&buff[buff_len], new_format, args);

    j = 0;  /* Counts quote characters in logic below */
    while (buff_len > MAX_C_LINE_LEN)
    {
        /*
        ** The buffer could contain embedded Newlines, in which case it is not
        ** necessarily too long to output in a single fprintf.  To simplify the
        ** logic, though, scan for Newlines within the line limit and if found,
        ** do a separate fprintf and update pointers.
        */
        cp = strchr(buff, '\n');
        if (cp != 0 && (cp - buff) <= MAX_C_LINE_LEN)
        {
            temp = *++cp;
            *cp = '\0';
            fprintf (fid, "%s", buff);
        }
        else
        {
#ifdef CONTINUATION_METHOD
            /*
            ** VAX C only allows the '\' continuation character in #defines,
            ** tokens, and character strings.  Since we have no way of knowing
            ** whether we're in a #define or a character string, we want to make
            ** sure we break the line in the middle of a token.  The code below
            ** assumes that we are guaranteed to find two consecutive token
            ** characters when we scan back in the buffer, and does the line
            ** break in between those two characters.
            */
            for (cp = &buff[MAX_C_LINE_LEN-1] ;; cp--)
                if (isidchar(*cp) && isidchar(cp[-1]))
                    break;
#else
            /*
            ** Scan backwards for a "break" char that is not in a literal string
            ** and output up through that break char followed by a newline.
            */
            char *dpos = NULL, *odpos, *endpos;

            /*
             * Find the rightmost delimiter and count quotes.
             */
            endpos = &buff[MAX_C_LINE_LEN-1];
            temp = *endpos;             /* Save end char */
            *endpos = '\0';             /* Temp make eos */

            cp = buff;
            while ((cp = strpbrk(cp, " .,-\"()")) != NULL)
            {
                if (*cp == '"')
                {
                    j++;
                    odpos = dpos;       /* Save delim pos before this quote */
                    dpos = cp;          /* Save as possible delimiter */
                }
                else if (*cp == '-' && *++cp != '>')
                    continue;
                else
                    dpos = cp;
                cp++;
            }

            *endpos = temp;             /* Restore end char */

            /*
             * Got the rightmost delimiter but if quote count is odd then it
             * is within a quoted string so back off to the previous delimiter
             * before the quote.
             */
            if (j%2 == 1 && odpos != NULL)
                dpos = odpos;

            if (dpos == NULL)
                /* Didn't find a break char; punt and output entire line. */
                cp = &buff[buff_len];
            else
                cp = dpos + 1;
#endif

            /*
            ** cp now points at the character after where we want to split.
            ** Put a temporary break in the buffer so we can print this section.
            */
            temp = *cp;
            *cp = '\0';
#ifdef CONTINUATION_METHOD
            fprintf (fid, "%s\\\n", buff);
#else
            fprintf (fid, "%s\n", buff);
#endif
        }

        /*
        ** Restore the saved character, update length and pointers, try again.
        */
        *cp = temp;
        buff_len = buff_len - ((long)cp - (long)buff);
        buff = cp;
    }

    /*
    ** Output the final part of the line.  buff points somewhere within
    ** out_buffer, at the start of the final part of the line.
    */
    if (buff[buff_len-1] == '\n')
    {
        /*
        ** The last character in the buffer is a newline.
        ** Output the buffer now and reinitialize buffer and index.
        */
        fprintf (fid, "%s", buff);
        for (buff_len = 0 ; buff_len < indent_sp ; obuff[buff_len++] = ' ')
            ;
    }
    else
    {
        /*
        ** The last character in the buffer is not a newline.
        ** If the current data in the buffer starts at other than
        ** out_buffer[0], shift it in the buffer so that it does.
        */
        if (buff != obuff)
        {
            i = buff_len;
            while (i-- > 0)
                *obuff++ = *buff++;
        }
    }
}

#endif
