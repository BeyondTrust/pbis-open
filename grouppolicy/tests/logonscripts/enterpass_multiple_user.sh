# $1: Number of enterpass.sh instances
# $2: Number of users to logon
# $3: Random number range

if [ $# -ne 3 ]; then
    echo "usage: program_name <num_of_iterations> <num_of_users> <random_num_range>"
    exit 1
fi

i=1

while [ $i -le $1 ]; do
    sh enterpass.sh $2 $3
    i=`expr $i + 1`
done

