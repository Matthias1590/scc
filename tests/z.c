int write(int fd, char *s, int len);

int main() {
    write(1, "hey!\n", 5);
    return 0;
}
