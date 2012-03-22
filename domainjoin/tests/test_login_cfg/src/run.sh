#!/bin/bash

GP_DIR=~snambakam/workspaces/Sapphire.meta/linux/i386/RPM/centeris-grouppolicy/staging/usr/centeris/

export LD_LIBRARY_PATH=$GP_DIR/lib:$LD_LIBRARY_PATH

#exec gdb ./test_login_cfg $*
#valgrind --show-reachable=yes --leak-check=full --log-file=/tmp/lwijoin.log ./test_login_cfg $*
./test_login_cfg $*
#strace ./test_login_cfg $*
