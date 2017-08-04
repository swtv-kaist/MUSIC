#!/bin/sh
# Execute all tests for this option

if test $# = 0; then
	echo "Usage: sh filename.sh executable-COMUT"
	echo "Error: no executable-COMUT file was given"
	exit 1
fi

echo "Executing tests for option -o"
echo "========================================="

for t in *.sh
do
    if test $t != $0 ; then
        sh $t $1
        echo "========================================="
    fi
done
