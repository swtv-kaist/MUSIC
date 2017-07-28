#!/bin/sh
# Usage: output-folder-directory

if test $# != 1; then
	echo "Usage: output-folder-directory"
	exit 1
fi

mkdir $1

mkdir ${1}/base-choice
mkdir ${1}/option-A
mkdir ${1}/option-B
mkdir ${1}/option-l
mkdir ${1}/option-m
mkdir ${1}/option-o
mkdir ${1}/option-re
mkdir ${1}/option-rs
mkdir ${1}/option-rs-re

for i in `seq 1 63`; do
	mkdir ${1}/option-A/case$i
	mkdir ${1}/option-B/case$i
done

mkdir ${1}/option-rs-re/case1
mkdir ${1}/option-rs-re/case2

# Usage: directory-to-create-folders text-file
make_case_folders()
{
	COUNT=0

	while read line; do
		COUNT=$((COUNT+1))
		mkdir ${1}/case$COUNT
	done < $2
}

make_case_folders ${1}/option-l option-l-error-test.txt
make_case_folders ${1}/option-o option-o-error-test.txt

make_case_folders ${1}/option-re option-re-error-test.txt
COUNT=$((COUNT+1))
mkdir ${1}/option-re/case$COUNT
COUNT=$((COUNT+1))
mkdir ${1}/option-re/case$COUNT

make_case_folders ${1}/option-rs option-rs-error-test.txt
COUNT=$((COUNT+1))
mkdir ${1}/option-rs/case$COUNT
COUNT=$((COUNT+1))
mkdir ${1}/option-rs/case$COUNT

while read line
do
	TEST_CASE=`echo -n $line | tail -c 4`
	mkdir ${1}/option-m/$TEST_CASE
done < option-m-all-op.txt