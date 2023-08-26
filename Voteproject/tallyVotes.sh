#!/bin/bash

tallyResultsFile=$1

if [[ "$#" -ne 1 ]]; then                                    
    echo "There must be exactly 1 argument in the command line!"
    exit 1
fi

if [[ ! -f inputFile.txt ]];then
    echo "File doesn't exist!"
    exit 1
fi

if ! [[ $(stat -c "%a" inputFile.txt) == "777" ]]; then
  echo "Permission denied!"
  exit 1
fi

awk '!a[$1, $2]++' inputFile.txt >$tallyResultsFile 
# read each line and takes 1,2 positions
# cancel duplicates names px kostas merkouris twice

cat $tallyResultsFile | awk '{print $3}' | sort | uniq -c >$tallyResultsFile
# counting 


