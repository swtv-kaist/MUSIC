#!/bin/sh
# When user provides an input file that does not exist
# COMUT exits and does not generate anything.

if test $# = 0; then
	echo "Usage: sh filename.sh executable-COMUT"
	echo "Error: no executable-COMUT file was given"
	exit 1
fi

# DIR: the directory that this script is in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Executing $0"

CURRENT_NUM_OF_FILES=`find . | wc -l`

# Run the tool with an input file that does not exist
$1 input-src/not-exist.c > /dev/null 2>&1

# The test success if exit value is NOT 0
# and no files are generated
if test $? != 0 && test `find . | wc -l` = $CURRENT_NUM_OF_FILES
then
    echo "[SUCCESS] invalid input file"
else
    echo "[FAIL] invalid input file"
fi

# Run the tool with an input file that does not exist
../../tool -m ssdl > /dev/null 2>&1

# The test success if exit value is NOT 0
# and no files are generated
if test $? != 0 && test `find . | wc -l` = $CURRENT_NUM_OF_FILES
then
    echo "[SUCCESS] no input file specified"
else
    echo "[FAIL] no input file specified"
fi