#!/bin/bash

GP_DIR=~snambakam/workspaces/Sapphire.meta/linux/i386/RPM/centeris-grouppolicy/staging/usr/centeris/

export LD_LIBRARY_PATH=$GP_DIR/lib:$LD_LIBRARY_PATH

#exec gdb ./lwi-domainjoin-cli $*
#valgrind --show-reachable=yes --leak-check=full --log-file=/tmp/lwijoin.log ./lwi-domainjoin-cli $*
./lwi-domainjoin-cli $*
#strace ./lwi-domainjoin-cli $*
