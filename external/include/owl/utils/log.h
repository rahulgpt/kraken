#ifndef owl_log_h
#define owl_log_h

#define owl_println(format, ...) printf(format "\n", ##__VA_ARGS__)
#define owl_log_ln(format, ...) printf(format "\n", ##__VA_ARGS__)

#define __get_macro(_1, _2, NAME, ...) NAME

// prints date, time, tag name, INFO in bold green color before actual message
#define __logI1(tag, ...)                          \
    printf("%s %s [%s]", __DATE__, __TIME__, tag); \
    printf(" \033[1;32mINFO\033[0m ");             \
    printf(__VA_ARGS__);                           \
    fflush(stdout);

// prints time, file name, line number, INFO in normal green color before actual message
#define __logI2(...)                                  \
    printf("%s %s:%d", __TIME__, __FILE__, __LINE__); \
    printf(" \033[0;32mINFO\033[0m ");                \
    printf(__VA_ARGS__);                              \
    fflush(stdout);

#define owl_log_info(...) __get_macro(__VA_ARGS__, __logI2, __logI1)(__VA_ARGS__)

// prints date, time, tag name, DEBUG in bold yellow color before actual message
#define __logD2(tag, ...)                          \
    printf("%s %s [%s]", __DATE__, __TIME__, tag); \
    printf(" \033[1;33mDEBUG\033[0m ");            \
    printf(__VA_ARGS__);                           \
    fflush(stdout);
// prints time, file name, line number, DEBUG in normal yellow color before actual message
#define __logD1(...)                                  \
    printf("%s %s:%d", __TIME__, __FILE__, __LINE__); \
    printf(" \033[0;33mDEBUG\033[0m ");               \
    printf(__VA_ARGS__);                              \
    fflush(stdout);

#define owl_log_debug(...) __get_macro(__VA_ARGS__, __logD2, __logD1)(__VA_ARGS__)

#endif /* owl_log_h */