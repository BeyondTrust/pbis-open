/*
 * GCC 4.x  complains about the spu_info as incomplete element type
 * spu_info is declared in /usr/include/machine/sys/getppdp.h
 */
#ifndef _MPINFO_T
#define _MPINFO_T
// Use an empty definition instead of copying from system headers
typedef union mpinfou {         /* For lint */
} mpinfou_t;
#endif /* _MPINFO_T */
