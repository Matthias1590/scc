#!/usr/bin/env python3
from compile import compile
import subprocess
import argparse

def main() -> None:
    parser = argparse.ArgumentParser(description="Compile a C source file using SCC, QBE, and GCC")
    parser.add_argument("src", help="The C source file to compile")
    parser.add_argument("-r", "--rm", help="Remove files after compilation", action="store_true")
    args = parser.parse_args()

    exit_code, output = compile(args.src, "out.elf", args.rm)
    if exit_code != 0:
        print(output, end="")
        exit(exit_code)

    exit_code, output = subprocess.getstatusoutput("./out.elf")
    print(output, end="")
    exit(exit_code)

if __name__ == "__main__":
    main()
