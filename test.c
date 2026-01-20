#include "test.h"

int main(int argc, char **argv) {
    while (*argv)
    {
        puts(*argv);
        argv = argv + 1;
    }
    return 0;
}
