/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
 *  OSF DCE Version 1.0 
 */
/*
**
**  NAME
**
**      dce_error.c
**
**  FACILITY:
**
**      Distributed Computing Environment (DCE)
**
**  ABSTRACT:
**
**  Error status management routines.
**
**
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_NL_TYPES_H
#include <nl_types.h>       /* public types for NLS (I18N) routines */
#else
#warning Message catalog support disabled
#endif /* HAVE_NL_TYPES_H */

#define FACILITY_CODE_MASK          0xF0000000
#define FACILITY_CODE_SHIFT         28

#define COMPONENT_CODE_MASK         0x0FFFF000
#define COMPONENT_CODE_SHIFT        12

#define STATUS_CODE_MASK            0x00000FFF
#define STATUS_CODE_SHIFT           0

#define NO_MESSAGE                  "THIS IS NOT A MESSAGE"

/*
 * The system-dependant location for the catalog files is defined in sysconf.h
 */

#ifndef RPC_DEFAULT_NLSPATH
#define RPC_DEFAULT_NLSPATH "/usr/lib/nls/msg/en_US.ISO8859-1/%s.cat"
/* #error Define RPC_DEFAULT_NLSPATH in your sysconf.h file. */
#endif

#ifndef RPC_NLS_FORMAT
#define RPC_NLS_FORMAT "%s.cat"
#endif


/*
**++
**
**  ROUTINE NAME:       dce_error_inq_text
**
**  SCOPE:              PUBLIC - declared in dce_error.h
**
**  DESCRIPTION:
**      
**  Returns a text string in a user provided buffer associated with a given 
**  error status code. In the case of errors a text string will also be 
**  returned indicating the nature of the error.
**
**  INPUTS:
**
**      status_to_convert   A DCE error status code to be converted to 
**                          text form.
**
**  INPUTS/OUTPUTS:         None.
**
**  OUTPUTS:
**
**      error_text          A user provided buffer to hold the text
**                          equivalent of status_to_convert or
**                          a message indicating what error occurred.
**                          
**
**      status              The result of the operation. One of:
**                           0  -  success
**                          -1  -  failure
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

static void dce_get_msg (status_to_convert, error_text, fname, cname, status)

unsigned long           status_to_convert;
unsigned char           *error_text;
unsigned char           *fname;
unsigned char           *cname;
int                     *status;

{
    unsigned short  facility_code;
    unsigned short  component_code;
    unsigned short  status_code;
#ifdef HAVE_NL_TYPES_H
    nl_catd     catd;
#endif
    char        component_name[4];
    char        *facility_name;
    char        filename_prefix[7];
    char        nls_filename[11];
    char        alt_filename[80];
    char        *message;
    static char alphabet[] = "abcdefghijklmnopqrstuvwxyz_0123456789-+@";
    static char *facility_names[] = {
        "dce",
        "dfs"
    };

    /*
     * set up output status for future error returns
     */
    if (status != NULL)
    {
        *status = -1;
    }
    
    /*
     * check for ok input status
     */
    if (status_to_convert == 0)
    {
        if (status != NULL)
        {
            *status = 0;
        }
        strcpy ((char *)error_text, "successful completion");
        return;
    }

    /*
     * extract the component, facility and status codes
     */
    facility_code = (status_to_convert & FACILITY_CODE_MASK)
        >> FACILITY_CODE_SHIFT;
        
    component_code = (status_to_convert & COMPONENT_CODE_MASK)
        >> COMPONENT_CODE_SHIFT;

    status_code = (status_to_convert & STATUS_CODE_MASK)
        >> STATUS_CODE_SHIFT;

    /*
     * see if this is a recognized facility
     */
    if (facility_code == 0 || facility_code > sizeof (facility_names) / sizeof (char *))
    {
        sprintf ((char *) error_text, "status %08lx (unknown facility)", status_to_convert);
        return; 
    }

    facility_name = facility_names[facility_code - 1];

    /*
     * Convert component name from RAD-50 component code.  (Mapping is:
     * 0 => 'a', ..., 25 => 'z', 26 => '{', 27 => '0', ..., 36 => '9'.)
     */

    component_name[3] = 0;
    component_name[2] = alphabet[component_code % 40];
    component_code /= 40;
    component_name[1] = alphabet[component_code % 40];
    component_name[0] = alphabet[component_code / 40];

    if (fname != NULL)
        sprintf ((char*) fname, "%3s", facility_name);
    if (cname != NULL)
        sprintf ((char*) cname, "%3s", component_name);

    sprintf ((char*) filename_prefix, "%3s%3s", facility_name, component_name);

    sprintf ((char*) nls_filename, RPC_NLS_FORMAT, filename_prefix);

    /*
     * Open the message file
     */
#ifdef HAVE_NL_TYPES_H
    catd = (nl_catd) catopen (nls_filename, 0);
    if (catd == (nl_catd) -1)
    {
        /*
         * If we did not succeed in opening message file using NLSPATH,
         * try to open the message file in a well-known default area
         */
         
        sprintf (alt_filename,
                 RPC_DEFAULT_NLSPATH,
                 filename_prefix);
        catd = (nl_catd) catopen (alt_filename, 0);
            
        if (catd == (nl_catd) -1)
        {
            sprintf ((char *) error_text, "status %08lx", status_to_convert);
            return;
        }
    }    

    /*
     * try to get the specified message from the file
     */
    message = (char *) catgets (catd, 1, status_code, NO_MESSAGE);


    /*
     * if everything went well, return the resulting message
     */
    if (strcmp (message, NO_MESSAGE) != 0)
    {
        sprintf ((char *) error_text, "%s", message);
        if (status != NULL)
        {
            *status = 0;
        }
    }
    else
    {
        sprintf ((char *) error_text, "status %08lx", status_to_convert);
    }

    catclose (catd);
#else
    sprintf ((char *) error_text, "status %08lx", status_to_convert);
#endif
}        
void dce_error_inq_text (
unsigned long           status_to_convert,
unsigned char           *error_text,
int                     *status
)
{
    char        cname[4];
    char        fname[4];

    /*
     * check for ok input status
     */
    if (status_to_convert == 0)
    {
        if (status != NULL)
        {
            *status = 0;
        }
        strcpy ((char *)error_text, "successful completion");
        return;
    }

    dce_get_msg (status_to_convert, error_text, fname, cname, status);
    strcat ((char*) error_text, " (");
    strcat ((char*) error_text, fname);
    strcat ((char*) error_text, " / ");
    strcat ((char*) error_text, cname);
    strcat ((char*) error_text, ")");
}

int dce_fprintf(FILE *f, unsigned long index, ...)
{
    va_list ap;
    int st;
    int i;
    char format[1024];

    dce_get_msg(index, format, NULL, NULL, &st);
    if (st != 0) return EOF;

    va_start(ap, index);
    i = vfprintf(f, format, ap);
    va_end(ap);
    return i;
}
int dce_printf(unsigned long index, ...)
{
    va_list ap;
    int st;
    int i;
    char format[1024];

    dce_get_msg(index, format, NULL, NULL, &st);
    if (st != 0) return EOF;

    va_start(ap, index);
    i = vfprintf(stdout, format, ap);
    va_end(ap);
    return i;
}

#ifdef BUILD_STCODE
main(int argc, char **argv)
{
    long code;
    int i;
    int _;
    char message[1024];

    if (argc <= 1) {
        printf("Usage:  stcode {0x<hex status code> | <decimal status code>}\n");
        exit(1);
    }

    for (i=1; i < argc; i++) {
        if(strncmp(argv[i], "0x", 2) == 0)
                sscanf(argv[i], "%x", &code);
        else
                sscanf(argv[i], "%d", &code);
        dce_error_inq_text(code, message, &_);
        printf("%d (decimal), %x (hex): %s\n", code, code, message);
    }
}
#endif
