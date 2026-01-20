#include "test.h"

int main(int argc, char **argv) {
    char c = 'A';
    write(1, &c, 1);
    // while (*argv)
    // {
    //     puts(*argv);
    //     argv = argv + 1;
    // }
    return 0;
}
