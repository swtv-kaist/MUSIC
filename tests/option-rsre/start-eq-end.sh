#!/bin/sh
# When user provides same start and end location for mutation range
# COMUT will generate no mutants. (only mutation database file)

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

# Run test on each input source file in input-src directory
for TEST_INPUT in *
do
    cd $DIR

    # make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "output/$TEST_INPUT" | sed 's/.\{2\}$//'`
    mkdir $OUTPUT_FOLDER_NAME
    
    echo "$1 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME -rs 1 1 -re 1 1"
    # Run the tool with the input source
    # and start and end of mutation range be start of file
    $1 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME -rs 1 1 -re 1 1 > /dev/null 2>&1

    MUTDB_NAME="`echo "$TEST_INPUT" | sed 's/.\{2\}$//'`_mut_db.out"

    # The test success if exit value is 0 (execution success)
    # and only an empty mutation database file is generated
    if test $? = 0 && test `ls $OUTPUT_FOLDER_NAME` = $MUTDB_NAME \
        && [ ! -s "${OUTPUT_FOLDER_NAME}/$MUTDB_NAME" ]
    then
        echo "[SUCCESS] $TEST_INPUT start is same as end"
    else
        echo "[FAIL] $TEST_INPUT start is same as end"
    fi
    
    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME

    cd input-src
done
