/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Modified by Likewise Software Corporation 2007.
 */

#ifndef __GPA_PRIMES_H__
#define __GPA_PRIMES_H__

#include "gpamissing.h"

GPA_BEGIN_DECLS

/* Prime numbers.
 */

/* This function returns prime numbers spaced by approximately 1.5-2.0
 * and is for use in resizing data structures which prefer
 * prime-valued sizes.	The closest spaced prime function returns the
 * next largest prime, or the highest it knows about which is about
 * MAXINT/4.
 */
gpauint	   gpa_spaced_primes_closest (gpauint num) GPA_GNUC_CONST;

GPA_END_DECLS

#endif /* __GPA_PRIMES_H__ */
