#!/bin/bash

if [ ! -e /var/lib/pbis ]
then
    mkdir -p -m 0755 /var/lib
    ln -s /opt/pbis/var /var/lib/pbis
fi

(
    # Wait for lwsmd to start.
    sleep 0.5

    while ! /opt/pbis/bin/lwsm list &> /dev/null
    do
        sleep 0.5
    done

) &

exec /opt/pbis/sbin/lwsmd "$@"
