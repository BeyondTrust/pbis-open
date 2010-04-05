#!/bin/bash

GP_DIR=~snambakam/workspaces/Sapphire.meta/linux/i386/RPM/centeris-grouppolicy/staging/usr/centeris/

export LD_LIBRARY_PATH=$GP_DIR/lib:$LD_LIBRARY_PATH

#exec gdb ./test_sec_user $*
#valgrind --show-reachable=yes --leak-check=full --log-file=/tmp/lwijoin.log ./test_sec_user $*
./test_sec_user $*
#strace ./test_sec_user $*
