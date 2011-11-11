#!/bin/bash
if ! /opt/pbis/bin/klist >/dev/null; then
	echo "You must use first kinit get credentials in the correct domain."
	exit 1
fi
host=$1
if [ -z "$host" ]; then
	echo "Specify the host to test against as the first argument"
	exit 1
fi
princ=$2
if [ -z "$princ" ]; then
	echo "Specify the service principal to use as the second argument"
	exit 1
fi
user=$3
password=$4

generate_size=100
calls=100

protocols()
{
	echo "$auth over tcp $protection protection$signing"
	echo /opt/pbis/tools/echo_client -h "$host" -a "$princ" -t -c$calls -g$generate_size $auth_options $signing_options $protection_options
	/opt/pbis/tools/echo_client -h "$host" -a "$princ" -t -c$calls -g$generate_size $auth_options $signing_options $protection_options || exit 2
	echo "$auth over named pipe $protection protection$signing"
	echo /opt/pbis/tools/echo_client -h "$host" -a "$princ" -n -e '\pipe\echo' -c$calls -g$generate_size $auth_options $signing_options $protection_options
	/opt/pbis/tools/echo_client -h "$host" -a "$princ" -n -e '\pipe\echo' -c$calls -g$generate_size $auth_options $signing_options $protection_options || exit 2
}

auth_methods()
{
	auth="Negotiate authentication"
	auth_options="-S negotiate"
	protocols

	auth="Kerberos authentication"
	auth_options="-S krb5"
	protocols

	auth="Ntlm authentication"
	auth_options="-S ntlm -U $user -P $password"
	protocols
}

signing_types()
{
	signing=""
	signing_options=""
	auth_methods

	signing=" with header signing"
	signing_options="-s"
	auth_methods
}

protection="integrity"
protection_options="-p5"
signing_types

protection="privacy"
protection_options="-p6"
signing_types
