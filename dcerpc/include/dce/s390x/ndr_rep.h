/*
 * s390/ndr_rep.h 
 *
 * platform dependent (OS + Architecture) file split out from stubbase.h 
 * for DCE 1.1 code cleanup.
 *
 * For IBM S/390 architecture - 32bit, Big Endian Mode
 *
 * This file contains the architecture specific definitions of the 
 * local scalar data representation used 
 *
 * This file is always included as part of stubbase.h 
 */

#ifndef _NDR_REP_H 
#define _NDR_REP_H

#define NDR_LOCAL_INT_REP     ndr_c_int_big_endian
#define NDR_LOCAL_FLOAT_REP   ndr_c_float_ieee
#define NDR_LOCAL_CHAR_REP    ndr_c_char_ascii

#endif /* _NDR_REP_H */
