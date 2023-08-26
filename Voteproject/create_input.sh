#!/bin/bash 

politicalParties=$1
numLines=$2

if [[ "$#" -ne 2 ]]; then
    echo "Must be exactly 2 args"
    exit 1
fi

#range3-12
if [[ "$numLines" -lt 1 ]]; then 
    echo "Invalid input for number of lines of the input file"
    exit 1
fi 

char=abcdefghigklmnopqrstuvwxyz
CHAR=ABCDEFGHIJKLMNOPQRSTUVWXYZ

for ((count=0; count<$numLines; count++))
do
    #firstname
    firstName="${CHAR:RANDOM%${#CHAR}:1}"
    num1=$((RANDOM%12+3))

    for (( count1=0; count1<num1;count1++))
    do
        firstName+="${char:RANDOM%${#char}:1}"
    done

    #lastname
    lastName="${CHAR:RANDOM%${#CHAR}:1}"
    num2=$((RANDOM%12+3))
    
    for (( count3=0; count3<$num2; count3++ ))
    do
        lastName+="${char:RANDOM%${#char}:1}"
    done


    political_choice=($(shuf -n 1 $politicalParties))
    echo "$firstName $lastName $political_choice">>inputFile.txt

done    

   
