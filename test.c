#include "test.h"

int main(int argc, char **argv) {
    while (argc != 0)
    {
        puts(*argv);
        argc = argc - 1;
        argv = argv + 2;
    }
    return argc;
}
