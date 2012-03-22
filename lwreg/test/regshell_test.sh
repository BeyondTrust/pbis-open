#!/bin/sh
#
# Test of regshell use from command line
#
regshell=/opt/pbis/bin/regshell

# Add values at root key level
#
$regshell ls '[HKEY_THIS_MACHINE\]'
echo ======================================================
$regshell add_value '[HKEY_THIS_MACHINE\]' string_value_1 REG_SZ "One: This is a string value: 1"
$regshell add_value '[HKEY_THIS_MACHINE\]' dword_value_1 REG_DWORD 12345678
$regshell add_value '[HKEY_THIS_MACHINE\]' binary_value_1 REG_BINARY 102993847566574839201
$regshell add_value '[HKEY_THIS_MACHINE\]' multisz_value_1 REG_MULTI_SZ "One" "Hello world" "ABCDEFG" '!@#ASDFasdf$1234'
$regshell ls '[HKEY_THIS_MACHINE\]'
echo ======================================================
$regshell delete_value '[HKEY_THIS_MACHINE\]' string_value_1
$regshell delete_value '[HKEY_THIS_MACHINE\]' dword_value_1
$regshell delete_value '[HKEY_THIS_MACHINE\]' binary_value_1
$regshell delete_value '[HKEY_THIS_MACHINE\]' multisz_value_1
$regshell ls '[HKEY_THIS_MACHINE\]'
echo ======================================================

# Add values at one subkey level
# 
$regshell add_key '[HKEY_THIS_MACHINE\test_keys_2]'
$regshell ls '[HKEY_THIS_MACHINE\]'
echo ======================================================
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2]' string_value_2 REG_SZ "Two: This is a string value: 1"
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2]' dword_value_2 REG_DWORD 23456782
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2]' binary_value_2 REG_BINARY 202993847566574839201
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2]' multisz_value_2 REG_MULTI_SZ "Two" "Hello world" "ABCDEFG" '!@#ASDFasdf$1234'
$regshell ls '[HKEY_THIS_MACHINE\test_keys_2]'
echo ======================================================

# Add values at a secondary subkey level
#
$regshell add_key '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]'
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]' string_value_3 REG_SZ "Three: This is a string value: 1"
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]' dword_value_3 REG_DWORD 34567890
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]' binary_value_3 REG_BINARY 302993847566574839201
$regshell add_value '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]' multisz_value_3 REG_MULTI_SZ "Three" "Hello world" "ABCDEFG" '!@#ASDFasdf$1234'
$regshell ls '[HKEY_THIS_MACHINE\test_keys_2\test_keys_3]'
echo ======================================================
$regshell ls '[HKEY_THIS_MACHINE\test_keys_2]'
echo ======================================================

# Cleanup the testing mess...
# 
$regshell delete_tree '[HKEY_THIS_MACHINE\test_keys_2]'
exit 1
