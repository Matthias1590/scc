#!/usr/bin/env python3
import subprocess
import argparse

QBE = "./qbe/qbe"
SCC = "./scc"

def main() -> None:
    parser = argparse.ArgumentParser(description="Compile a C source file using SCC, QBE, and GCC")
    parser.add_argument("src", help="The C source file to compile")
    parser.add_argument("-o", "--output", help="The output executable path (default: out.elf)", default="out.elf")
    parser.add_argument("-r", "--rm", help="Remove intermediate files after compilation", action="store_true")
    args = parser.parse_args()

    exit_code, output = compile(args.src, args.output, args.rm)
    print(output, end="")
    exit(exit_code)

def compile(src: str, output: str, rm: bool) -> tuple[int, str]:
    def _rm(*files: str) -> None:
        if not rm:
            return
        for file in files:
            try:
                subprocess.run(["rm", "-f", file], check=True)
            except subprocess.CalledProcessError:
                pass

    # Compile source to qbe ir using scc
    compile_exit_code, compile_output = scc(src)
    if compile_exit_code != 0:
        return compile_exit_code, compile_output

    # Compile qbe ir to assembly using qbe
    qbe_exit_code, qbe_output = qbe("out.qbe", "out.s")
    if qbe_exit_code != 0:
        _rm("out.qbe", "out.s")
        return qbe_exit_code, qbe_output

    # Compile assembly to executable using gcc
    gcc_exit_code, gcc_output = gcc("out.s", output)
    if gcc_exit_code != 0:
        _rm("out.qbe", "out.s", output)
        return gcc_exit_code, gcc_output

    _rm("out.qbe", "out.s")
    return 0, ""

def scc(src: str) -> tuple[int, str]:
    e, o = subprocess.getstatusoutput(f"{SCC} {src}")
    o += '\n'
    return e, o

def qbe(input_qbe: str, output_asm: str) -> tuple[int, str]:
    e, o = subprocess.getstatusoutput(f"{QBE} -o {output_asm} {input_qbe}")
    o += '\n'
    return e, o

def gcc(input_asm: str, output_exe: str) -> tuple[int, str]:
    e, o = subprocess.getstatusoutput(f"gcc -o {output_exe} {input_asm}")
    o += '\n'
    return e, o

if __name__ == "__main__":
    main()
