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

case $# in 
2) ;;
*) echo 'Usage: perf_tcp.sh server_host_name client_program_directory' 1>&2 ; exit 1;;
esac

cd $2
##IP=`host $1 | awk '{print $3}'`
# Use the dce getip program, and if that doesn't work, try the
# basic approach of greping /etc/hosts.  This combination
# should cover most any platform.
if [ -x /opt/dcelocal/bin/getip ]; then
        IP=`getip $1`
else
        IP=`grep $1 /etc/hosts | awk '{print $1}'`
fi

FAILED=""

echo "client 0a"
client 0 "ncacn_ip_tcp:${IP}[2001]" 3 40 y y || FAILED="$FAILED 0a"
echo "client 0b"
client 0 "ncacn_ip_tcp:${IP}[2001]" 3 40 y n || FAILED="$FAILED 0b"
echo "client 0c"
client 0 "ncacn_ip_tcp:${IP}[2001]" 3 40 n y || FAILED="$FAILED 0c"
echo "client 0d"
client 0 "ncacn_ip_tcp:${IP}[2001]" 3 40 n n || FAILED="$FAILED 0d"
echo "client 1a"
client 1 "ncacn_ip_tcp:${IP}[2001]" 3 40 y y 400 || FAILED="$FAILED 1a"
echo "client 1b"
client 1 "ncacn_ip_tcp:${IP}[2001]" 3 40 y n 400 || FAILED="$FAILED 1b"
echo "client 1c"
client 1 "ncacn_ip_tcp:${IP}[2001]" 3 10 y y 4000 || FAILED="$FAILED 1c"
echo "client 1d"
client 1 "ncacn_ip_tcp:${IP}[2001]" 3 10 y n 4000 || FAILED="$FAILED 1d"
#echo "client 1e"
#client 1 "ncacn_ip_tcp:${IP}[2001]" 3 2 y y 100000 || FAILED="$FAILED 1e"
#echo "client 1f"
#client 1 "ncacn_ip_tcp:${IP}[2001]" 3 2 y n 100000 || FAILED="$FAILED 1f"
echo "client 2a"
client 2 "ncacn_ip_tcp:${IP}[2001]" 3 100 y y 400 || FAILED="$FAILED 2a"
echo "client 2b"
client 2 "ncacn_ip_tcp:${IP}[2001]" 3 100 y n 400 || FAILED="$FAILED 2b"
echo "client 2c"
client 2 "ncacn_ip_tcp:${IP}[2001]" 3 10 y y 4000 || FAILED="$FAILED 2c"
echo "client 2d"
client 2 "ncacn_ip_tcp:${IP}[2001]" 3 10 y n 4000 || FAILED="$FAILED 2d"
#echo "client 2e"
#client 2 "ncacn_ip_tcp:${IP}[2001]" 3 2 y y 100000 || FAILED="$FAILED 2e"
#echo "client 2f"
#client 2 "ncacn_ip_tcp:${IP}[2001]" 3 2 y n 100000 || FAILED="$FAILED 2f"
#echo "client 3"
#client 3 "ncacn_ip_tcp" || FAILED="$FAILED 3"
echo "client 4"
client 4 "ncacn_ip_tcp:${IP}[2001]" 3 2 || FAILED="$FAILED 4"
#echo "client 5"
#client 5 "ncacn_ip_tcp" || FAILED="$FAILED 5"
echo "client 6a"
client 6 "ncacn_ip_tcp:${IP}[2001]" 3 100 y y || FAILED="$FAILED 6a"
echo "client 6b"
client 6 "ncacn_ip_tcp:${IP}[2001]" 3 100 y n || FAILED="$FAILED 6b"
echo "client 8"
client 8 "ncacn_ip_tcp:${IP}[2001]" y || FAILED="$FAILED 8"
echo "client 7"
client 7 "ncacn_ip_tcp:${IP}[2001]" || FAILED="$FAILED 7"
echo "client 9"
client 9 "ncacn_ip_tcp:${IP}[2001]" || FAILED="$FAILED 9"
echo "client 10a"
client 10 "ncacn_ip_tcp:${IP}[2001]" 4 3 y y 2 || FAILED="$FAILED 10a"
echo "client 10b"
client 10 "ncacn_ip_tcp:${IP}[2001]" 2 3 y n 2 || FAILED="$FAILED 10b"
echo "client 10c"
client 10 "ncacn_ip_tcp:${IP}[2001]" 4 3 y y 2 1 || FAILED="$FAILED 10c"
echo "client 10d"
client 10 "ncacn_ip_tcp:${IP}[2001]" 2 3 y n 2 1 || FAILED="$FAILED 10d"
echo "client 10e"
client 10 "ncacn_ip_tcp:${IP}[2001]" 4 3 y y 2 2 || FAILED="$FAILED 10e"
echo "client 10f"
client 10 "ncacn_ip_tcp:${IP}[2001]" 2 3 y n 2 2 || FAILED="$FAILED 10f"
# These test are unsupported and will fail
#echo "client 12a"
#client 12 "ncacn_ip_tcp:${IP}[2001]" 2 10 y
#echo "client 12b"
#client 12 "ncacn_ip_tcp:${IP}[2001]" 2 10 n
echo "client 13"
client 13 "ncacn_ip_tcp:${IP}[2001]" || FAILED="$FAILED 13"
echo "client 14a"
client 14 "ncacn_ip_tcp:${IP}[2001]" 4 n 1 || FAILED="$FAILED 14a"
echo "client 14b"
client 14 "ncacn_ip_tcp:${IP}[2001]" 4 y 1 || FAILED="$FAILED 14b"
echo "client 15a"
client 15 "ncacn_ip_tcp:${IP}[2001]" 2 y 1 || FAILED="$FAILED 15a"
echo "client 15b"
client 15 "ncacn_ip_tcp:${IP}[2001]" 2 n 1 || FAILED="$FAILED 15b"
echo "client 15c"
client 15 "ncacn_ip_tcp:${IP}[2001]" 2 y 1 5 || FAILED="$FAILED 15c"
echo "client 15d"
client 15 "ncacn_ip_tcp:${IP}[2001]" 2 n 1 5 || FAILED="$FAILED 15d"

if [ -n "$FAILED" ] ; then
	echo "The failed tests were"
	echo "$FAILED"
	exit 1
else
	echo "All tests OK"
	exit 0
fi

