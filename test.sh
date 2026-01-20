#!/bin/bash

set -e
make -B
set +e
echo 

# Find all .c files in the tests directory
for file in tests/*.c; do
    # Compile and run using run.sh, redirect stdout but keep stderr
    ./run.sh "$file" DONTRUNMAKE > /dev/null

    # If the exit code isn't 0, print an error message
    if [ $? -ne 0 ]; then
        echo -e "\033[31m[!] Test failed for $file\033[0m"
    else
        echo -e "\033[32m[+] Test passed for $file\033[0m"
    fi
done
