/* klugey def'n of a socket address struct helps hide IPV4 vs. IPV6 ugliness */

typedef union sockaddr_u {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
    struct sockaddr_in6 sa_in6;
} sockaddr_t;

/* sockaddr.h ends here */
