#!/bin/bash

poll_log=$1

if [[ "$#" -ne 1 ]]; then                                    
    echo "There must be exactly 1 argument in the command line!"
    exit 1
fi

if [[ ! -f $poll_log ]];then
    echo "File doesn't exist!"
    exit 1
fi

if ! [[ $(stat -c "%a" $poll_log) == "777" ]]; then
  echo "Permission denied!"
  exit 1
fi

awk '!a[$1, $2]++' $poll_log  >pollerResultsFile.txt
cat pollerResultsFile.txt | awk '{print $3}' | sort | uniq -c >pollerResultsFile.txt