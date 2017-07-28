#!/bin/sh
# WARNING: 	This script will clear all files (not folders) in directory named output
#			This directory contains files generated from last execution of run-test.sh
# 			Backup anything needed before running test

success_or_fail()
{
	if test $? = 0; then
		printf "[SUCCESS] "
	else
		printf "[FAIL] "
	fi
}

# this function compares 2 directories file by file
compare_dir() 
{
	# this function must take 2 arguments
	if test $# != 2; then
		echo "Error: compare_dir must take 2 directory arguments"
		exit 1
	fi

	if test ! -d $1; then
		echo "Error: cannot find dir $1"
		exit 1
	fi

	if test ! -d $2; then
		echo "Error: cannot find dir $2"
		exit 1
	fi	

	# check number of files in each directory
	NUM_OF_FILE_1=`find $1 -type f -name \* | wc -l`
	NUM_OF_FILE_2=`find $2 -type f -name \* | wc -l`

	if test $NUM_OF_FILE_1 != $NUM_OF_FILE_2; then
		echo "number of files are different: $1 $2"
		return 1
	fi

	# compare file by file
	for SINGLE_FILE in `ls $1`; do
		diff --brief ${1}/$SINGLE_FILE ${2}/$SINGLE_FILE >/dev/null 2>&1
		RESULT=$?

		if test $RESULT = 2; then
			echo "$SINGLE_FILE does not exist in both $1 and $2"
			return 1
		elif test $RESULT = 1; then
			echo "$SINGLE_FILE in $1 and $2 are different"
			return 1
		fi
	done

	return 0
}

test_option_o()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		
		$1 input-src/$TEST_INPUT $LINE -o output/${OUTPUT_FOLDER_NAME}/option-o/case$COUNT \
		$BASE_L $BASE_RS $BASE_RE $BASE_M_A_B > /dev/null

		compare_dir output/${OUTPUT_FOLDER_NAME}/option-o/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-o/case$COUNT

		success_or_fail

		echo "$1 input-src/$TEST_INPUT $LINE -o output/${OUTPUT_FOLDER_NAME}/option-o/case$COUNT $BASE_L $BASE_RS $BASE_RE $BASE_M_A_B"
	done < option-o-error-test.txt
}

test_option_rs()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
		$BASE_L $LINE $BASE_RE $BASE_M_A_B > /dev/null 2>&1

		compare_dir output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT

		success_or_fail

		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-o/case$COUNT $BASE_L $LINE $BASE_RE $BASE_M_A_B"
	done < option-rs-error-test.txt

	# test case exceeding EOF
	# exceed line number
	RS="-rs $((3+LAST_LINE_NUMBER)) $LAST_LINE_COL_NUMBER"
	COUNT=$((COUNT+1))

	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
	$BASE_L $RS $BASE_RE $BASE_M_A_B > /dev/null 2>&1

	compare_dir output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT $BASE_L $RS $BASE_RE $BASE_M_A_B"

	# exceed column number
	RS="-rs $LAST_LINE_NUMBER $((3+LAST_LINE_COL_NUMBER))"
	COUNT=$((COUNT+1))

	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
	$BASE_L $RS $BASE_RE $BASE_M_A_B > /dev/null 2>&1
 
	compare_dir output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs/case$COUNT $BASE_L $RS $BASE_RE $BASE_M_A_B"
}

test_option_re()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
		$BASE_L $BASE_RS $LINE $BASE_M_A_B > /dev/null 2>&1

		compare_dir output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT

		success_or_fail

		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT $BASE_L $BASE_RS $LINE $BASE_M_A_B"
	done < option-re-error-test.txt

	# test case exceeding EOF
	# exceed line number
	RE="-re $((3+LAST_LINE_NUMBER)) $LAST_LINE_COL_NUMBER"
	COUNT=$((COUNT+1))

	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
	$BASE_L $BASE_RS $RE $BASE_M_A_B > /dev/null 2>&1

	compare_dir output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT $BASE_L $BASE_RS $RE $BASE_M_A_B"

	# exceed column number
	RE="-re $LAST_LINE_NUMBER $((3+LAST_LINE_COL_NUMBER))"
	COUNT=$((COUNT+1))

	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
	$BASE_L $BASE_RS $RE $BASE_M_A_B > /dev/null 2>&1

	compare_dir output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-re/case$COUNT $BASE_L $BASE_RS $RE $BASE_M_A_B"
}

test_option_rs_re()
{
	# start and end of range of mutation are the same
	RS="-rs 1 1"
	RE="-re 1 1"
	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs-re/case1 \
	$BASE_L	$RS $RE $BASE_M_A_B > /dev/null 2>&1

	compare_dir output/${OUTPUT_FOLDER_NAME}/option-rs-re/case1 \
				expected-output/${OUTPUT_FOLDER_NAME}/option-rs-re/case1

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs-re/case1 $BASE_L	$RS $RE $BASE_M_A_B"

	# start and end of range of mutation are the same
	RS="-rs $LAST_LINE_NUMBER $LAST_LINE_COL_NUMBER"
	RE="-re 1 1"
	$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs-re/case2 \
	$BASE_L	$RS $RE $BASE_M_A_B > /dev/null 2>&1

	compare_dir output/${OUTPUT_FOLDER_NAME}/option-rs-re/case2 \
				expected-output/${OUTPUT_FOLDER_NAME}/option-rs-re/case2

	success_or_fail
	echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-rs-re/case2 $BASE_L	$RS $RE $BASE_M_A_B"
}

test_option_A()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT \
		$BASE_L $BASE_RS $BASE_RE $LINE > /dev/null
		compare_dir output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT
		success_or_fail
		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT $BASE_L $BASE_RS $BASE_RE $LINE"
	done < option-A-invalid-input.txt

	while read LINE
	do
		COUNT=$((COUNT+1))
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT \
		$BASE_L $BASE_RS $BASE_RE $LINE > /dev/null
		compare_dir output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT
		success_or_fail
		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-A/case$COUNT $BASE_L $BASE_RS $BASE_RE $LINE"
	done < option-A-unsupported-input.txt
}

test_option_B()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT \
		$BASE_L $BASE_RS $BASE_RE $LINE > /dev/null
		compare_dir output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT
		success_or_fail
		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT $BASE_L $BASE_RS $BASE_RE $LINE"
	done < option-B-invalid-input.txt

	while read LINE
	do
		COUNT=$((COUNT+1))
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT \
		$BASE_L $BASE_RS $BASE_RE $LINE > /dev/null
		compare_dir output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT
		success_or_fail
		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-B/case$COUNT $BASE_L $BASE_RS $BASE_RE $LINE"
	done < option-B-unsupported-input.txt
}

test_option_l()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		
		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-l/case$COUNT \
		$LINE $BASE_RS $BASE_RE $BASE_M_A_B > /dev/null 2>&1

		compare_dir output/${OUTPUT_FOLDER_NAME}/option-l/case$COUNT \
					expected-output/${OUTPUT_FOLDER_NAME}/option-l/case$COUNT

		success_or_fail

		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-l/case$COUNT $LINE $BASE_RS $BASE_RE $BASE_M_A_B"
	done < option-l-error-test.txt
}

test_option_m()
{
	COUNT=0

	while read LINE
	do
		COUNT=$((COUNT+1))
		OPERATOR=`echo -n $LINE | tail -c 4`

		$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-m/$OPERATOR \
		$BASE_L $BASE_RS $BASE_RE $LINE > /dev/null 2>&1

		compare_dir output/${OUTPUT_FOLDER_NAME}/option-m/$OPERATOR \
					expected-output/${OUTPUT_FOLDER_NAME}/option-m/$OPERATOR
		success_or_fail
		echo "$1 input-src/$TEST_INPUT -o output/${OUTPUT_FOLDER_NAME}/option-m/$OPERATOR $BASE_L $BASE_RS $BASE_RE $LINE"
	done < option-m-all-op.txt
}

# =========================== main =============================
if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

# remove all files in directory named output
find output/ -type f | xargs rm > /dev/null 2>&1

# DIR: the directory that this script exist in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for TEST_INPUT in `ls input-src`; do
	echo "Executing tests on source file $TEST_INPUT"
	echo "================================================================="

	# Getting line and column number of EOF
	LAST_LINE_NUMBER=`grep -c ^ input-src/$TEST_INPUT`
	LAST_LINE="`tail -1 input-src/$TEST_INPUT`"
	LAST_LINE_COL_NUMBER=$((1+`expr length $LAST_LINE`))

	# make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "$TEST_INPUT" | sed 's/.\{2\}$//'`

    # base choice
	BASE_M_A_B="-m oaan -A + -B -"
	BASE_L="-l 10"
	BASE_RS="-rs 1 1"
	BASE_RE="-re $LAST_LINE_NUMBER $LAST_LINE_COL_NUMBER"
	BASE_O="-o output/${OUTPUT_FOLDER_NAME}/base-choice"

	# COMMAND="$1 input-src/$TEST_INPUT $BASE_O $BASE_L $BASE_RS $BASE_RE $BASE_M_A_B"

	echo "Testing option -o"
	test_option_o $1
	echo "======================"

	echo "Testing option -l"
	test_option_l $1
	echo "======================"

	echo "Testing option -rs"
	test_option_rs $1
	echo "======================"

	echo "Testing option -re"
	test_option_re $1
	echo "======================"

	echo "Testing option -rs -re together"
	test_option_rs_re $1
	echo "======================"

	echo "Testing option -A"
	test_option_A $1
	echo "======================"

	echo "Testing option -B"
	test_option_B $1
	echo "======================"

	echo "Testing option -m"
	test_option_m $1
	echo "======================"
	
	echo "================================================================="
done