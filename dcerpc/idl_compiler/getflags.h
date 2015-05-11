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
**
**  NAME
**
**      GETFLAGS.H
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Constants for command line parsing
**
**  VERSION: DCE 1.0
**
*/

#ifndef GETFLAGS_H
#define GETFLAGS_H

#include <nidl.h>


typedef char *FLAGDEST;
typedef struct options
        {
        char *option;
        int ftype;
        FLAGDEST dest;
        } OPTIONS;

/*
 * Rico, 23-Mar-90: The format of the ftype field appears to be:
 *
 *      Bit 15:   (1 bit)  HIDARG flag - flags hidden args to printflags fn
 *      Bit 14:   (1 bit)  VARARG flag - flags a variable option (can repeat)
 *      Bit 13-8: (6 bits) maximum number of occurences of a variable option
 *      Bit  7-0: (8 bits) argument type
 *
 *       15  14  13              8   7                    0
 *      -----------------------------------------------------
 *      | H | V |  max_occurences  |     argument_type      |
 *      -----------------------------------------------------
 */

/* Argument types */
#define INTARG      0
#define STRARG      1
#define TOGGLEARG   2
#define CHRARG      3
#define FLTARG      4
#define LONGARG     5
#define ASSERTARG   6
#define DENYARG     7
#define OSTRARG     8           /* Optional string arg, added 23-Mar-90 */

#define HIDARG (128 << 8)       /* H bit */
#define VARARGFLAG 64           /* V bit - gets shifted 8 bits by macros */
#define MULTARGMASK 63          /* Mask to get max_occurences */

/* Macros for specifying ftype */
#define MULTARG(n, a) (((n) << 8) + a)
#define AINTARG(n) MULTARG(n,INTARG)
#define VINTARG(n) AINTARG(n|VARARGFLAG)
#define ASTRARG(n) MULTARG(n,STRARG)
#define VSTRARG(n) ASTRARG(n|VARARGFLAG)
#define ATOGGLEARG(n) MULTARG(n,TOGGLEARG)
#define AASSERTARG(n) MULTARG(n,ASSERTARG)
#define ADENYARG(n) MULTARG(n,DENYARG)
#define ACHRARG(n) MULTARG(n,CHRARG)
#define VCHRARG(n) ACHRARG(n|VARARGFLAG)
#define AFLTARG(n) MULTARG(n,FLTARG)
#define VFLTARG(n) AFLTARG(n|VARARGFLAG)
#define ALONGARG(n) MULTARG(n,LONGARG)
#define VLONGARG(n) AFLTARG(n|VARARGFLAG)

/* Macros for converting command line arguments */
#define GETINT(s) {s = atoi(*++av); ac--;}
#define GETSTR(s) {s = *++av;ac--;}
#define GETCH(s) {av++; s = av[0][0]; ac--;}
#define GETFLT(s) {s = atof(*++av); ac--;}
#define GETLONG(s) {s = atol(*++av); ac--;}


void printflags (
    OPTIONS table[]
);

void getflags (
    int argc,
    char **argv,
    OPTIONS table[]
);

void flags_incr_count (
    OPTIONS table[],
    char *option,
    int delta
);

int flags_option_count (
    OPTIONS table[],
    char *option
);

int flags_other_count (
    void
);

char *flags_other (
    int index
);


#endif
