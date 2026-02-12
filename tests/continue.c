int main(void) {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        if (i == 5) {
            sum += 10;
            i++;
            continue;
        }
        sum += i;
        i++;
    }

    if (sum != 50) {
        return 1;
    }
    return 0;
}
