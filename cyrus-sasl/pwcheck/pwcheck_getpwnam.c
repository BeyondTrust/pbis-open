/* pwcheck_getpwnam.c -- check passwords using getpwname()
   $Id: pwcheck_getpwnam.c,v 1.1 1999/08/26 16:22:43 leg Exp $

Copyright 1998, 1999 Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Carnegie Mellon
University not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
******************************************************************/

#include <pwd.h>

extern char *crypt();

char *pwcheck(userid, password)
char *userid;
char *password;
{
    char* r;
    struct passwd *pwd;

    pwd = getpwnam(userid);
    if (!pwd) {
	r = "Userid not found";
    }
    else if (pwd->pw_passwd[0] == '*') {
	r = "Account disabled";
    }
    else if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) {
	r = "Incorrect password";
    }
    else {
	r = "OK";
    }

    endpwent();
    
    return r;
}
