#!/usr/bin/env awk -f
#
# Copyright (c) Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the MakeKit project nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#

##
#
# help.awk -- formats --help output
#
##

BEGIN {
    x=0;
    indent=30;
    wrap=80;
}

/^Options/ {
    if (NR != 1)
    {
        printf("\n")
    }
    print
    next
}

/###/ {
    printf("\n");
    x=0
    next
}

{
    if (x == 0)
    {
        printf("  %s", $0);
        if (length($0) + 2 >= indent)
        {
            printf("\n");
            x = 0;
        }
        else
        {
            x = length($0) + 2;
        }
        
        for (; x < indent; x++)
        {
            printf(" ");
        }
    }
    else
    {
        for (i = 1; i <= NF; i++)
        {
            if (x + length($i) >= wrap)
            {
                printf("\n")
                for (x = 0; x < indent; x++)
                {
                    printf(" ");
                }
            }
            else if (x != indent)
            {
                printf(" ");
                x++
            }

            printf("%s", $i);
            x += length($i);
        }
    }
}

