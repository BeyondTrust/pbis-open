#!/bin/sh
tmpreg=/tmp/regup-$$.txt
BIN_DIR=/opt/likewise/bin
LWREGSHELL=$BIN_DIR/lwregshell
PSTORE_UPGRADE=$BIN_DIR/psupgrade

#verify tools needed to perform registry upgrade are present
if [ ! -x $LWREGSHELL ]; then
  exit 1
fi
if [ ! -x $PSTORE_UPGRADE ]; then
  exit 1
fi

# Export the existing registry, in legacy format
$LWREGSHELL export --legacy $tmpreg
if [ ! -s $tmpreg ]; then
  exit 1
fi

# "Rename" relevant pstore entries to new registry location
$PSTORE_UPGRADE $tmpreg > ${tmpreg}.out
if [ ! -s ${tmpreg}.out ]; then
  rm -f $tmpreg
  rm -f ${tmpreg}.out
  exit 0
fi

# Import renamed pstore entries
$LWREGSHELL import ${tmpreg}.out
rm -f ${tmpreg}.out

# Clear out old pstore entries
# Using "." to avoid quadruple backslashes
if [ `grep -c '\[HKEY_THIS_MACHINE.Services.lsass.Parameters.Providers.ActiveDirectory.Pstore.Default\]' $tmpreg` -gt 0 ]; then
  $LWREGSHELL delete_tree '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\Pstore\Default]'
fi

# Remove values with identical default attributes from registry
# 
rm -f $tmpreg
