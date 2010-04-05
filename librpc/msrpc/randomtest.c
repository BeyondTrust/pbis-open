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
#include <stdlib.h>
#include <string.h>

#include <random.h>


void printhex(unsigned char *m, size_t len)
{
	int i;
	printf("hash: ");

	for (i = 0; i < len; i++) {
		printf("%02x", m[i]);
	}

	printf("\n");
}


int main(int argc, char *argv[])
{
	int count;
	unsigned char *buf = NULL;
	char *str = NULL;

	if (argc < 2) {
		printf("Error: No buffer length\n");
		return -1;
	}

	count = atoi(argv[1]);
	if (count <= 0) {
		printf("Error: Invalid buffer length\n");
		return -1;
	}

	buf = (unsigned char*) malloc(sizeof(unsigned char) * count);
	if (buf == NULL) {
		printf("Error: Memory allocation error\n");
		return -1;
	}

	get_random_buffer(buf, count);
	printf("buffer length: %d\n", count);
	printhex(buf, count);

	get_random_string(str, count);
	printf("buffer length: %d\n", count);
	printf("string: %s\n", str);

	free(buf);
	free(str);

	return 0;
}
