int printf(char *__format, ...);

int main(void) {
    int x = 123;
    char *y = "hey";
    printf("Hello! %d\n%s\n", x, y);
    return 0;
}
