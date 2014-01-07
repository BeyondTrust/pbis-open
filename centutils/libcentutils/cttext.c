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

#include "config.h"
#include "ctbase.h"

DWORD
CTTextFormatParagraph(
    const char* source,
    unsigned int indent_first,
    unsigned int indent,
    unsigned int max_width,
    char** dest
    )
{
    DWORD ceError;
    StringBuffer buffer;

    char* cursor = (char*) source;
    unsigned int lineSize = 0;
    unsigned int i;

    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferConstruct(&buffer));

    for (i = 0; i < indent_first; i++)
    {
	BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(&buffer, ' '));
	lineSize++;
    }
    
    // Eliminate leading whitespace
    while (*cursor && isspace((int) *cursor))
	cursor++;

    while (*cursor)
    {
	char* begin = cursor;
	char* spaceStart, *spaceEnd;

	// Find any whitespace
	while (*cursor && isspace((int) *cursor))
	{
	    cursor++;
	}


	// Save whitespace location
	spaceStart = begin;
	spaceEnd = cursor;


	// If we don't have a remaining non-space segment, there's no point in continuing
	if (!*cursor)
	    break;

	// Find non-whitespace segment
	begin = cursor;
	while (*cursor && !isspace((int) *cursor))
	{
	    cursor++;
	}

	// Decide if we need to wrap (print newline + indent) or just copy whitespace
	if ((cursor - begin) + (spaceEnd - spaceStart) + lineSize > max_width)
	{
	    BAIL_ON_CENTERIS_ERROR(
		ceError = CTStringBufferAppendChar(&buffer, '\n'));
	    
	    lineSize = 0;

	    for (i = 0; i < indent; i++)
	    {
		BAIL_ON_CENTERIS_ERROR(
		    ceError = CTStringBufferAppendChar(&buffer, ' '));
		lineSize++;
	    }
	}
	else
	{
	    BAIL_ON_CENTERIS_ERROR(
		ceError = CTStringBufferAppendLength(&buffer, spaceStart, spaceEnd-spaceStart));
	    lineSize += spaceEnd-spaceStart;
	}

	BAIL_ON_CENTERIS_ERROR(
	    ceError = CTStringBufferAppendLength(&buffer, begin, cursor-begin));
	
	lineSize += (cursor - begin);
    }

    *dest = CTStringBufferFreeze(&buffer);

error:
    CTStringBufferDestroy(&buffer);

    return ceError;
}
