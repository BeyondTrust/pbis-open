#!/bin/sh
#
# 
# (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
# (c) Copyright 1989 HEWLETT-PACKARD COMPANY
# (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
# To anyone who acknowledges that this file is provided "AS IS"
# without any express or implied warranty:
#                 permission to use, copy, modify, and distribute this
# file for any purpose is hereby granted without fee, provided that
# the above copyright notices and this notice appears in all source
# code copies, and that none of the names of Open Software
# Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
# Corporation be used in advertising or publicity pertaining to
# distribution of the software without specific, written prior
# permission.  Neither Open Software Foundation, Inc., Hewlett-
# Packard Company, nor Digital Equipment Corporation makes any
# representations about the suitability of this software for any
# purpose.
# 
#

#
# Unix Bourne shell script for running perf client tests
# The parameter passed in is a string binding.  

# Usage:
#           run_client.sh `string binding returned by server`
#
# An example run of this test involves starting the server:
#        ./server 1 ncadg_ip_udp
#              ==> returns binding, such as ncadg_ip_udp:129.35.130.133[2001]
#         run_client.sh ncadg_ip_udp:129.35.130.133[2001]
#

CLIENT=client
DEBUG=
PC=false

for ARG
do
    case $ARG in
    -d)
        DEBUG="-d"
        shift 
        ;;
    -pc)
        PC=true
        shift 
        ;;
    *) 
        break
        ;;
    esac
done

echo "Testing against $1..."

protocol=`expr "$1" : "\(.*\):"`

${CLIENT} ${DEBUG} 0 $1 3 400 y n                # Null call  
${CLIENT} ${DEBUG} 0 $1 3 400 y y                # Null idempotent call
${CLIENT} ${DEBUG} 1 $1 3 50 y n 3000            # Ins 
${CLIENT} ${DEBUG} 1 $1 3 50 y y 3000            # Ins, idempotent
${CLIENT} ${DEBUG} 2 $1 3 50 y n 3000            # Outs
${CLIENT} ${DEBUG} 2 $1 3 50 y y 3000            # Outs idempotent
if [ $PC != "true" ]
then
    ${CLIENT} ${DEBUG} 1 $1 3 3 y n 100000       # Ins (100K)
    ${CLIENT} ${DEBUG} 1 $1 3 3 y y 100000       # Ins, idempotent (100K)
    ${CLIENT} ${DEBUG} 2 $1 3 3 y n 100000       # Outs (100K)
    ${CLIENT} ${DEBUG} 2 $1 3 3 y y 100000       # Outs idempotent (100K)
fi

# Note: test 3 only works with UDP
if [ "$protocol" = "ncadg_ip_udp" ]
then
    ${CLIENT} ${DEBUG} 3 ncadg_ip_udp                # Broadcast
fi

${CLIENT} ${DEBUG} 4 $1 3 2                         # Maybe

# Note: test 5 only works with UDP
if [ "$protocol" = "ncadg_ip_udp" ]
then
    ${CLIENT} ${DEBUG} 5 ncadg_ip_udp                # Broadcast/maybe
fi
${CLIENT} ${DEBUG} 6 $1 3 100 y y                # Floating point
${CLIENT} ${DEBUG} 6 $1 3 100 y n                # Floating point
${CLIENT} ${DEBUG} 7 $1                          # Unregistered interface
${CLIENT} ${DEBUG} 8 $1 n                        # Forwarding
${CLIENT} ${DEBUG} 9 $1                          # Exception
${CLIENT} ${DEBUG} 10 $1 2 2 y n 60              # Slow call
${CLIENT} ${DEBUG} 10 $1 2 2 y y 60              # Slow idempotent call
