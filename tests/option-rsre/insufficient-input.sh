#!/bin/sh
# When user provides 0 or 1 number only for option -rs -re
# COMUT exits on error

if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

test_insufficient_input()
{
	# if second parameter is 0 then run test with no input for rs/re
	# else run test with 1 input
	if test $2 = 0 ; then
		LOCATION=""
	else
		LOCATION="1"
	fi

	mkdir $OUTPUT_FOLDER_NAME
    
    # Run the tool with the input source and line/col number 0
    echo "$3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $LOCATION" 
    $3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $LOCATION > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] ${TEST_INPUT} $1 not enough input"
    else
        echo "[FAIL] $TEST_INPUT $1 $2 not enough input"
    fi
    
    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME
}

# DIR: the directory that this script is in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Executing $0"
cd input-src

# Exit script if there is no input source file
if test `find . -type f -name \* | wc -l` = 0 ; then
    exit
fi

for TEST_INPUT in *
do
	cd $DIR

    # make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "output/$TEST_INPUT" | sed 's/.\{2\}$//'`

    test_insufficient_input "-rs" 0 $1
    test_insufficient_input "-rs" 1 $1
    test_insufficient_input "-re" 0 $1
    test_insufficient_input "-re" 1 $1

    cd input-src
done