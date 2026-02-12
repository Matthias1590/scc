#!/usr/bin/env python3
from test import test
import os

def main() -> None:
    test_files = [f for f in os.listdir("tests") if f.endswith(".c")]
    failed_tests = []
    for test_file in test_files:
        test_file = os.path.join("tests", test_file)
        error = test(test_file)
        if error is not None:
            print(f"\033[31mTest {test_file} failed:\033[0m\n{error}")
            failed_tests.append((test_file, error))
        else:
            print(f"\033[32mTest {test_file} passed\033[0m")
        print()
    if len(failed_tests) == 0:
        print(f"\033[32mAll {len(test_files)} tests passed!\033[0m")
    else:
        print(f"\033[31m{len(failed_tests)} out of {len(test_files)} tests failed\033[0m")

if __name__ == "__main__":
    main()
