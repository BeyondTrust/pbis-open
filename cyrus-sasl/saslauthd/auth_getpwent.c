/* MODULE: auth_getpwent */

/* COPYRIGHT
 * Copyright (c) 1997-2000 Messaging Direct Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MESSAGING DIRECT LTD. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MESSAGING DIRECT LTD. OR
 * ITS EMPLOYEES OR AGENTS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * END COPYRIGHT */

/* SYNOPSIS
 * crypt(3) based passwd file validation
 * END SYNOPSIS */

#ifdef __GNUC__
#ident "$Id: auth_getpwent.c,v 1.7 2005/01/27 04:39:52 shadow Exp $"
#endif

/* PUBLIC DEPENDENCIES */
#include "mechanisms.h"
#include <unistd.h>
#include <string.h>
#include <pwd.h>

# ifdef WITH_DES
#  ifdef WITH_SSL_DES
#   include <openssl/des.h>
#  else
#   include <des.h>
#  endif /* WITH_SSL_DES */
# endif /* WITH_DES */

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
/* END PUBLIC DEPENDENCIES */

#define RETURN(x) return strdup(x)

/* FUNCTION: auth_getpwent */

char *					/* R: allocated response string */
auth_getpwent (
  /* PARAMETERS */
  const char *login,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service __attribute__((unused)),
  const char *realm __attribute__((unused))
  /* END PARAMETERS */
  )
{
    /* VARIABLES */
    struct passwd *pw;			/* pointer to passwd file entry */
    /* END VARIABLES */
  
    pw = getpwnam(login);
    endpwent();

    if (pw == NULL) {
	RETURN("NO");
    }

    if (strcmp(pw->pw_passwd, (const char *)crypt(password, pw->pw_passwd))) {
	RETURN("NO");
    }

    RETURN("OK");
}

/* END FUNCTION: auth_getpwent */

/* END MODULE: auth_getpwent */
