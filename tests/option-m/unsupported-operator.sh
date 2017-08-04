#!/bin/sh
# When user provides an operator name that COMUT does not support
# COMUT exits on error

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
    
    # Run the tool with the input source and operator STRP
    ../../tool input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME -m STRP > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] $TEST_INPUT specify unsupported operator"
    else
        echo "[FAIL] $TEST_INPUT specify unsupported operator"
    fi
    
    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME

    cd input-src
done
