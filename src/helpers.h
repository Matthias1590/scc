#pragma once

#ifdef DEBUG
    #define trace(...) printf(__VA_ARGS__)
#else
    #define trace(...)
#endif

#define unused(x) (void)(x)

#define assert(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
            abort(); \
        } \
    } while (0)

#define todo(msg) \
    do { \
        fprintf(stderr, "TODO: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        abort(); \
    } while (0)

#define unreachable() \
    do { \
        fprintf(stderr, "Unreachable code reached (%s:%d)\n", __FILE__, __LINE__); \
        abort(); \
    } while (0)

char *read_file(const char *path);
