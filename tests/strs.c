int write(int fd, char *c, int count);

int  ft_strlen(char *str)
{
    int i = 0;
    while (str[i++])
        ;
    return (i - 1);
}

void    ft_putstr(char *str)
{
    write(1, str, ft_strlen(str));
}

void    ft_putchar(char c)
{
    write(1, &c, 1);
}

int main() {
    char *strs[3];
    strs[0] = "Hey\n";
    strs[1] = "This is a test\n";
    strs[2] = "Goodbye\n";

    int i = 0;
    while (i < 3) {
        ft_putstr(strs[i]);
        i++;
    }

    return 0;
}
