/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          
   Copyright (C) 2005, Mutsuo Saito,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include <stdio.h>
#include "mt19937ar.h"

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

/* initializes mt[N] with a seed */
void mt_init_genrand(mt* m, uint32_t s)
{
    m->mt[0]= s & 0xffffffffUL;
    for (m->mti=1; m->mti<N; m->mti++) {
        m->mt[m->mti] = 
	    (1812433253UL * (m->mt[m->mti-1] ^ (m->mt[m->mti-1] >> 30)) + m->mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array m->mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        m->mt[m->mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void mt_init_by_array(mt* m, uint32_t init_key[], int key_length)
{
    int i, j, k;
    mt_init_genrand(m, 19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        m->mt[i] = (m->mt[i] ^ ((m->mt[i-1] ^ (m->mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        m->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { m->mt[0] = m->mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        m->mt[i] = (m->mt[i] ^ ((m->mt[i-1] ^ (m->mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        m->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { m->mt[0] = m->mt[N-1]; i=1; }
    }

    m->mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
uint32_t mt_genrand_int32(mt* m)
{
    uint32_t y;
    static uint32_t mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (m->mti >= N) { /* generate N words at one time */
        int kk;

        if (m->mti == N+1)   /* if init_genrand() has not been called, */
            mt_init_genrand(m, 5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (m->mt[kk]&UPPER_MASK)|(m->mt[kk+1]&LOWER_MASK);
            m->mt[kk] = m->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (m->mt[kk]&UPPER_MASK)|(m->mt[kk+1]&LOWER_MASK);
            m->mt[kk] = m->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (m->mt[N-1]&UPPER_MASK)|(m->mt[0]&LOWER_MASK);
        m->mt[N-1] = m->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        m->mti = 0;
    }
  
    y = m->mt[m->mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
int32_t mt_genrand_int31(mt* m)
{
    return (long)(mt_genrand_int32(m)>>1);
}

/* generates a random number on [0,1]-real-interval */
double mt_genrand_real1(mt* m)
{
    return mt_genrand_int32(m)*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double mt_genrand_real2(mt* m)
{
    return mt_genrand_int32(m)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double mt_genrand_real3(mt* m)
{
    return (((double)mt_genrand_int32(m)) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double mt_genrand_res53(mt* m) 
{ 
    uint32_t a=mt_genrand_int32(m)>>5, b=mt_genrand_int32(m)>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */
