#!/bin/bash

set -e
make -B
set +e
echo 

num_tests=0
num_passed=0
num_failed=0

# Find all .c files in the tests directory
for file in tests/*.c; do
    num_tests=$((num_tests + 1))

    # Compile and run using run.sh, redirect stdout but keep stderr
    ./run.sh "$file" - > run.out
    code=$?

    # If the exit code isn't 0, print an error message
    if [ $code -ne 0 ]; then
        echo -e "\033[31m[!] Test exited abnormally for $file\033[0m"
        num_failed=$((num_failed + 1))
        continue
    fi

    out="${file%.c}.out"
    if [[ -f $out ]]; then
        diff run.out $out
        if [ $? -ne 0 ]; then
            echo -e "\033[31m[!] Test produced unexpected output for $file\033[0m"
            num_failed=$((num_failed + 1))
            continue
        fi
        rm run.out
    fi

    echo -e "\033[32m[+] Test passed for $file\033[0m"
    num_passed=$((num_passed + 1))
done

echo
if [ $num_failed -ne 0 ]; then
    echo -e "\033[31m[!] $num_failed out of $num_tests tests failed\033[0m"
else
    echo -e "\033[32m[+] All $num_tests tests passed\033[0m"
fi
