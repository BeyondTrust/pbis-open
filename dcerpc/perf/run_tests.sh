#!/bin/sh

echo "Starting server in the background"

./server 1 ncacn_ip_tcp &
server_pid=$!
sleep 2

echo "TCP Tests"

if ! ./perf_tcp.sh localhost . 
then
	rc=1
else
	rc=0
fi

kill $server_pid

exit $rc
