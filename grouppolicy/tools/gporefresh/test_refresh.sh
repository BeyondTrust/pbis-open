#!/bin/bash

NUMBER_OF_ITERATIONS=1
SLEEP_SECONDS=5

if [ $# -lt 1 ]; then
   echo "Usage:   test_refresh [# iterations] [sleep seconds]"
   echo "Example: test_refresh 10 5"
   echo "Note:    Number of iterations should be at least 1."
   echo "         Sleep seconds should be at least 5." 
   echo "         Default sleep is $SLEEP_SECONDS seconds."
   exit 1
fi

NUMBER_OF_ITERATIONS=$1

if [ $NUMBER_OF_ITERATIONS -lt 1 ]; then
   echo "Please choose the number of iterations to be at least 1"
   exit 1
fi

if [ $# -gt 1 ]; then
   SLEEP_SECONDS=$2
fi

if [ $SLEEP_SECONDS -lt 5 ]; then
   echo "Please specify at least 5 seconds to sleep between iterations."
   exit 1
fi

echo "Number of iterations:     $NUMBER_OF_ITERATIONS"
echo "Sleep between iterations: $SLEEP_SECONDS seconds"

for i in `seq 1 $NUMBER_OF_ITERATIONS`
do
echo $i
rm -f /etc/sudoers
/usr/centeris/bin/gporefresh
sleep $SLEEP_SECONDS
if [ ! -f /etc/sudoers ]; then
echo "Failed to download sudoers file"
exit 1
fi
done
