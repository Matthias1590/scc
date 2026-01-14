int factrec(int x);

int fact(int x) {
    return factrec(x);
}

int factrec(int x) {
    if (x == 0) {
        return 1;
    } else {
        return x * factrec(x - 1);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    return fact(5);
}
