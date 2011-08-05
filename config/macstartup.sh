#!/bin/bash

if [ ! -e /var/lib/likewise ]
then
    mkdir -p -m 0755 /var/lib
    ln -s /opt/likewise/var /var/lib/likewise
fi

(
    # Wait for lwsmd to start.
    sleep 0.5

    while ! /opt/likewise/bin/lwsm list &> /dev/null
    do
        sleep 0.5
    done

    /opt/likewise/bin/lwsm autostart
) &

exec /opt/likewise/sbin/lwsmd "$@"
