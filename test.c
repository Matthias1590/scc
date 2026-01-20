#include "test.h"

int main(int argc, char **argv) {
    puts("HEllo!\"\n");
    while (*argv)
    {
        puts(*argv);
        argv = argv + 1;
    }
    return 0;
}
