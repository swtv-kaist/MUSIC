#!/bin/sh
# COMUT exits on error in which user provides line/col real number for option rs/re

if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

test_real_number()
{
	if test $2 = "line" ; then
		LOCATION="5.0 1"
	else
		LOCATION="1 5.0"
	fi

	mkdir $OUTPUT_FOLDER_NAME
    
    # Run the tool with the input source and real number line/col
    echo "$3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $LOCATION" 
    $3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $LOCATION > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] ${TEST_INPUT} $1 $2 is a real number"
    else
        echo "[FAIL] $TEST_INPUT $1 $2 is a real number"
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

    # Make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "output/$TEST_INPUT" | sed 's/.\{2\}$//'`

    test_real_number "-rs" "line" $1
    test_real_number "-rs" "col" $1
    test_real_number "-re" "line" $1
    test_real_number "-re" "col" $1

    cd input-src
done