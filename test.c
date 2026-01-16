int puts(char *s);

int main(int argc, char **argv) {
    {
        int argc = 5;
        int x = argc;
    }
    puts("Hey!");
    puts(*argv);
    return argc;
}
