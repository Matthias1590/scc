#!/usr/bin/env python3
from compile import compile
from difflib import unified_diff as Diff
import subprocess
import argparse
import os

def diff(expected: str, actual: str) -> str | None:
    expected_lines = expected.splitlines(keepends=True)
    actual_lines = actual.splitlines(keepends=True)
    # Use ANSI color codes for colored diff
    if expected == actual:
        return None
    diff = list(Diff(expected_lines, actual_lines, fromfile='expected', tofile='actual'))
    if len(diff) == 0:
        return None
    colored = []
    for line in diff:
        if line.startswith('+++'):
            colored.append('\033[32m' + line + '\033[0m')  # Green
            continue
        elif line.startswith('---'):
            colored.append('\033[31m' + line + '\033[0m')  # Red
            continue
        # Show $ at end of each line for clarity
        line_ending = ''
        if line.endswith('\n'):
            line_ending = '\n'
            line = line[:-1]
        line += '␤' if line_ending else ''
        if line.startswith('+') and not line.startswith('+++'):
            colored.append('\033[32m' + line + '\033[0m' + line_ending)  # Green for additions
        elif line.startswith('-') and not line.startswith('---'):
            colored.append('\033[31m' + line + '\033[0m' + line_ending)  # Red for deletions
        elif line.startswith('@@'):
            colored.append('\033[36m' + line + '\033[0m' + line_ending)  # Cyan for hunk headers
        else:
            colored.append(line + line_ending)
    return ''.join(colored)

def read_file(path: str) -> str:
    with open(path, "r") as f:
        return f.read()

def test_source(src: str, compiler_output_path: str, rm) -> str | None:
    exit_code, compiler_output = compile(src, "out.elf", rm)
    expected_compile_code = 1 if os.path.exists(compiler_output_path) else 0
    if exit_code != expected_compile_code:
        return f"Expected compile exit code {expected_compile_code}, got {exit_code}"
    if os.path.exists(compiler_output_path):
        diff_error = diff(read_file(compiler_output_path), compiler_output)
        if diff_error is not None:
            return f"Compiler output differs:\n{diff_error}"
    return None

def test_executable(executable: str, expected_output_path: str) -> str | None:
    exit_code, output = subprocess.getstatusoutput(executable)
    output += '\n'
    if exit_code != 0:
        return f"Expected executable to exit with code 0, got {exit_code}"
    diff_error = diff(read_file(expected_output_path), output)
    if diff_error is not None:
        return f"Executable output differs:\n{diff_error}"
    return None

def test(src: str) -> str | None:
    expected_compile_output = os.path.splitext(src)[0] + ".error"
    expected_executable_output = os.path.splitext(src)[0] + ".out"
    compile_error = test_source(src, expected_compile_output, rm=False)
    if compile_error is not None:
        return compile_error
    if os.path.exists(expected_executable_output):
        executable_error = test_executable("./out.elf", expected_executable_output)
        if executable_error is not None:
            return executable_error
    return None

def main() -> None:
    parser = argparse.ArgumentParser(description="Test the compiler with a source file")
    parser.add_argument("src", help="Path to the source file to test")
    args = parser.parse_args()

    if not os.path.exists(args.src):
        print(f"Source file {args.src} does not exist")
        exit(1)

    error = test(args.src)
    if error is not None:
        print(f"\033[31mTest {args.src} failed:\033[0m")
        print(error)
        exit(1)

    print(f"\033[32mTest {args.src} passed\033[0m")
    exit(0)

if __name__ == "__main__":
    main()
