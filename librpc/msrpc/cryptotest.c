/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <stdio.h>
#include <string.h>

#include <md5.h>
#include <crypto.h>
#include <rc4.h>


void printdigest(char *printed, unsigned char *d, size_t dlen)
{
	int i;

	for (i = 0; i < dlen; i++) {
		snprintf(&printed[2*i], 3, "%02x", d[i]);
	}
}


int main()
{
	const char *pass = "TestPassword";

	struct md5context ctx;
	unsigned char passbuf[532] = {0};
	unsigned char printedbuf[1065];  /* 2*532 + 1 */
	unsigned char initval[16], digested_sess_key[16];
	size_t sess_key_len = 16;
	unsigned char sess_key[16] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	EncodePassBuffer(passbuf, pass);

	memset(initval, 0, sizeof(initval));

	md5init(&ctx);
	md5update(&ctx, initval, 16);
	md5update(&ctx, sess_key, sess_key_len);
	md5final(&ctx, digested_sess_key);

	rc4(passbuf, 516, digested_sess_key, 16);
	memcpy(&passbuf[516], initval, 16);

	printdigest((char*)printedbuf, passbuf, sizeof(passbuf));
	printedbuf[1064] = 0;

	printf("rc4: %s\n", printedbuf);

	return 0;
}
