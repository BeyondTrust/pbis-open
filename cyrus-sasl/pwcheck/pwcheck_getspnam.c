/* pwcheck_getspnam.c -- check passwords using getspnam()
   $Id: pwcheck_getspnam.c,v 1.1 1999/08/26 16:22:44 leg Exp $

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

#include <shadow.h>

extern char *crypt();

char *pwcheck(userid, password)
char *userid;
char *password;
{
    struct spwd *pwd;

    pwd = getspnam(userid);
    if (!pwd) {
	return "Userid not found";
    }
    
    if (strcmp(pwd->sp_pwdp, crypt(password, pwd->sp_pwdp)) != 0) {
	return "Incorrect password";
    }
    else {
	return "OK";
    }
}
