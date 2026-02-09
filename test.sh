#!/bin/bash

set -e
make -B
set +e
echo 

# Find all .c files in the tests directory
for file in tests/*.c; do
    # Compile and run using run.sh, redirect stdout but keep stderr
    ./run.sh "$file" - > run.out
    code=$?

    # If the exit code isn't 0, print an error message
    if [ $code -ne 0 ]; then
        echo -e "\033[31m[!] Test exited abnormally for $file\033[0m"
        continue
    else
        echo -e "\033[32m[+] Test passed for $file\033[0m"
        continue
    fi

    out="${file%.c}.out"
    if [[ -f $out ]]; then
        diff run.out $out
        if [ $? -ne 0 ]; then
            echo -e "\033[31m[!] Test produced unexpected output for $file\033[0m"
            continue
        fi
    fi
done
