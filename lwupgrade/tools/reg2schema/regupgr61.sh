#!/bin/sh
tmpreg=/tmp/regup-$$.txt
BIN_DIR=/opt/pbis/bin
LWREGSHELL=$BIN_DIR/regshell
PSTORE_UPGRADE=/opt/pbis/libexec/reg61sed.sh

get_dns_domain()
{
  infile=$1
  grep '^"DomainDnsName"=' $infile | \
       sed -e 's|DomainDnsName.*=||' -e 's|"||g' | \
       tr -d '\r'
}


if [ "$1" != "--install" ]; then
  echo "WARNING: This tool is called during system installation"
  echo "         and upgrade, and should not be called by an end-user."
  exit 1
fi

# Verify tools needed to perform registry upgrade are present
if [ ! -x $LWREGSHELL ]; then
  exit 1
fi
if [ ! -x $PSTORE_UPGRADE ]; then
  exit 1
fi

# Export the existing registry, in legacy format
$LWREGSHELL export --legacy - | sed 's/^#//' > $tmpreg
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


# Need to add TrustEnumerationWait commands if they don't exist - there should be 2 settings
DOMAIN_NAME=`get_dns_domain ${tmpreg} | tr '[a-z]' '[A-Z]'`
TRUSTWAITCOUNT=`grep -c TrustEnumerationWait ${tmpreg}`
if [ $TRUSTWAITCOUNT -lt 2 ]; then
    $LWREGSHELL  add_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\DomainJoin\'$DOMAIN_NAME']' TrustEnumerationWait REG_DWORD 0
    $LWREGSHELL  add_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\DomainJoin\'$DOMAIN_NAME']' TrustEnumerationWaitSeconds REG_DWORD 0
fi

# Clear out old pstore entries
# Using "." to avoid quadruple backslashes
if [ `grep -c '\[HKEY_THIS_MACHINE.Services.lsass.Parameters.Providers.ActiveDirectory.Pstore.Default\]' $tmpreg` -gt 0 ]; then
  $LWREGSHELL delete_tree '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\Pstore\Default]'
fi

# Remove values with identical default attributes from registry
# 
rm -f $tmpreg
