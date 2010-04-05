# $1: Number of user logons
# $2: Random number range

if [ $# -ne 2 ]; then
    echo "usage: program_name <num_of_users> <random_num_range>"
    exit 1
fi

i=1

while [ $i -le $1 ]; do
    export username=`printf usr%d $i`; 
    export PASSWORD=span1234;

    SECS=`expr $RANDOM % $2`;

    i=`expr $i + 1`

    echo "Logging in centerisads\\\\$username for $SECS seconds "
    ./enterpass-linux-i386 ssh centerisads\\$username@localhost sleep $SECS  || { echo stopped at $username; continue; };
    echo "Logging out centerisads\\\\$username "
done

