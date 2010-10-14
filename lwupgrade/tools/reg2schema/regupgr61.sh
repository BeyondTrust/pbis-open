#!/bin/sh
tmpreg=/tmp/regup-$$.txt
BIN_DIR=/opt/likewise/bin
LWREGSHELL=$BIN_DIR/lwregshell
LWSM=$BIN_DIR/lwsm
PSTORE_UPGRADE=$BIN_DIR/psupgrade

#verify tools needed to perform registry upgrade are present
if [ ! -x $LWSM ]; then
  exit 1
fi
if [ ! -x $LWREGSHELL ]; then
  exit 1
fi
if [ ! -x $PSTORE_UPGRADE ]; then
  exit 1
fi

# Shutdown everything, then bring up just lwregd for upgrade
# Temporary until this is integrated into the install script
# ==========================================================
$LWSM stop lwreg
$LWSM start lwreg
# ==========================================================


# Export the existing registry, in legacy format
$LWREGSHELL export --legacy $tmpreg
if [ ! -s $tmpreg ]; then
  exit 1
fi

# "Rename" relevant pstore entries to new registry location
$PSTORE_UPGRADE $tmpreg > ${tmpreg}.out
if [ ! -s ${tmpreg}.out ]; then
  rm -f $tmpreg
  exit 1
fi

# Import renamed pstore entries
$LWREGSHELL import ${tmpreg}.out
rm -f ${tmpreg}.out

# Clear out old pstore entries
$LWREGSHELL delete_tree '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\Pstore\Default]'


# Remove values with identical default attributes from registry
# 

# Restart lsassd
# ==========================================================
$LWSM start lsass
