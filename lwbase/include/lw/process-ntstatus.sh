#
# Scriptlet to process ntstatus.h
#

grep "#define LW_STATUS_" ntstatus.h  | awk ' { print $2 }' | perl -n -e ' chomp($_); $a = $_; $a =~ s/^LW_//; print sprintf("#define %-50s %s\n", $a, $_);'
