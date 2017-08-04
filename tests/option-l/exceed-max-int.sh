#!/bin/sh
# COMUT exits on error in which user provides a number larger than MAX_INT
# as input for option -l
# MAX_INT in C++ is 2147483647

if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

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
    mkdir $OUTPUT_FOLDER_NAME
    
    # Run the tool with the input source and input number larger than MAX_INT for option -l
    $1 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME -l 2147483648 > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] $TEST_INPUT input exceeds 2147483647"
    else
        echo "[FAIL] $TEST_INPUT input exceeds 2147483647"
    fi
    
    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME

    cd input-src
done

