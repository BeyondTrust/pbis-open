#!/bin/bash

GP_DIR=~snambakam/workspaces/Sapphire.meta/linux/i386/RPM/centeris-grouppolicy/staging/usr/centeris/

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GP_DIR/lib

exec gdb ./test_dhcpname_change $*
