int sum(int n, int arr[]) {
    int total = 0;
    int i = 0;
    while (i < n) {
        total += arr[i];
        i++;
    }
    return total;
}

int main(void) {
    int arr[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    if (sum(3, arr) != 6) {
        return 1;
    }
    return 0;
}
