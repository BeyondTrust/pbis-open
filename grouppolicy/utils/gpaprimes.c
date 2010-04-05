/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Modified by Likewise Software Corporation 2007.
 */

/* 
 * MT safe
 */

#include "includes.h"


static const gpauint gpa_primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};

static const gpauint gpa_nprimes = sizeof (gpa_primes) / sizeof (gpa_primes[0]);

gpauint
gpa_spaced_primes_closest (gpauint num)
{
  gpaint i;

  for (i = 0; i < gpa_nprimes; i++)
    if (gpa_primes[i] > num)
      return gpa_primes[i];

  return gpa_primes[gpa_nprimes - 1];
}

/*
#define __LWG_PRIMES_C__
#include "galiasdef.c"
*/
