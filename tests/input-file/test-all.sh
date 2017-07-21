#!/bin/sh
# Execute all tests for this option

echo "Executing tests for input file"
echo "========================================="

for t in *.sh
do
    if test $t != $0 ; then
        sh $t
        echo "========================================="
    fi
done
