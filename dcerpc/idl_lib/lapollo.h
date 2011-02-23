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
**  NAME:
**
**      lapollo.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Apollo system dependencies.
**
**  VERSION: DCE 1.0
*/

#ifndef LAPOLLO_H
#define LAPOLLO_H
    
/*
 * If we're building an apollo shared library, we need to use the shared
 * vesions of the functions in libc.  The following include file will
 * do the right thing.
 */
#ifdef APOLLO_GLOBAL_LIBRARY
#   include <local/shlib.h>
#endif

#include <stdlib.h>         

/*
 * Tell the compiler to place all static data, declared within the scope
 * of a function, in a section named nck_pure_data$.  This section will
 * be loaded as a R/O, shared, initialized data section.  All other data,
 * global or statics at the file scope, will be loaded as R/W, per-process,
 * and zero-filled.
 */
#   pragma HP_SECTION( , nck_pure_data$)

#endif
