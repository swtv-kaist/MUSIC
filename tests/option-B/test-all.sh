#!/bin/sh
# Execute all tests for this option

echo "Executing tests for option -B"
echo "========================================="

for t in *.sh
do
    if test $t != $0 ; then
        sh $t
        echo "========================================="
    fi
done
