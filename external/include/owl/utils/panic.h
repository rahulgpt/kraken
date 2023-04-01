#ifndef owl_panic_h
#define owl_panic_h

#define owl_panic(_msg_)                                                     \
    {                                                                        \
        fprintf(stderr, "panic: %s (%s:%d)\n", (_msg_), __FILE__, __LINE__); \
        exit(1);                                                             \
    }

#endif /* owl_panic_h */