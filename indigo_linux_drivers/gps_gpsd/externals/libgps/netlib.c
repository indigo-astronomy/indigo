/*
 * This file is Copyright (c) 2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <string.h>
#include <fcntl.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifndef AF_UNSPEC
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#endif /* AF_UNSPEC */
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif /* HAVE_SYS_UN_H */
#ifndef INADDR_ANY
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#endif /* INADDR_ANY */
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>     /* for htons() and friends */
#endif /* HAVE_ARPA_INET_H */
#include <unistd.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/ip.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gpsd.h"
#include "sockaddr.h"

/* work around the unfinished ipv6 implementation on hurd and OSX <10.6 */
#ifndef IPV6_TCLASS
# if defined(__GNU__)
#  define IPV6_TCLASS 61
# elif defined(__APPLE__)
#  define IPV6_TCLASS 36
# endif
#endif

socket_t netlib_connectsock(int af, const char *host, const char *service,
			    const char *protocol)
{
    struct protoent *ppe;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int ret, type, proto, one = 1;
    socket_t s;
    bool bind_me;

    INVALIDATE_SOCKET(s);
    ppe = getprotobyname(protocol);
    if (strcmp(protocol, "udp") == 0) {
	type = SOCK_DGRAM;
	proto = (ppe) ? ppe->p_proto : IPPROTO_UDP;
    } else {
	type = SOCK_STREAM;
	proto = (ppe) ? ppe->p_proto : IPPROTO_TCP;
    }

    /* we probably ought to pass this in as an explicit flag argument */
    bind_me = (type == SOCK_DGRAM);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = af;
    hints.ai_socktype = type;
    hints.ai_protocol = proto;
    if (bind_me)
	hints.ai_flags = AI_PASSIVE;
    if ((ret = getaddrinfo(host, service, &hints, &result))) {
	return NL_NOHOST;
    }

    /*
     * From getaddrinfo(3):
     *     Normally, the application should try using the addresses in the
     *     order in which they are returned.  The sorting function used within
     *     getaddrinfo() is defined in RFC 3484).
     * From RFC 3484 (Section 10.3):
     *     The default policy table gives IPv6 addresses higher precedence than
     *     IPv4 addresses.
     * Thus, with the default parameters, we get IPv6 addresses first.
     */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
	ret = NL_NOCONNECT;
	if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0)
	    ret = NL_NOSOCK;
	else if (setsockopt
		 (s, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
		  sizeof(one)) == -1) {
	    ret = NL_NOSOCKOPT;
	} else {
	    if (bind_me) {
		if (bind(s, rp->ai_addr, rp->ai_addrlen) == 0) {
		    ret = 0;
		    break;
		}
	    } else {
		if (connect(s, rp->ai_addr, rp->ai_addrlen) == 0) {
		    ret = 0;
		    break;
		}
	    }
	}

	if (!BAD_SOCKET(s)) {
#ifdef HAVE_WINSOCK2_H
	  (void)closesocket(s);
#else
	  (void)close(s);
#endif
	}
    }
    freeaddrinfo(result);
    if (ret != 0 || BAD_SOCKET(s))
	return ret;

#ifdef IPTOS_LOWDELAY
    {
	int opt = IPTOS_LOWDELAY;
	(void)setsockopt(s, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
#ifdef IPV6_TCLASS
	(void)setsockopt(s, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt));
#endif
    }
#endif
#ifdef TCP_NODELAY
    /*
     * This is a good performance enhancement when the socket is going to
     * be used to pass a lot of short commands.  It prevents them from being
     * delayed by the Nagle algorithm until they can be aggreagated into
     * a large packet.  See https://en.wikipedia.org/wiki/Nagle%27s_algorithm
     * for discussion.
     */
    if (type == SOCK_STREAM)
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
#endif

    /* set socket to noblocking */
#ifdef HAVE_FCNTL
    (void)fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
#elif defined(HAVE_WINSOCK2_H)
    u_long one1 = 1;
    (void)ioctlsocket(s, FIONBIO, &one1);
#endif
    return s;
}


const char *netlib_errstr(const int err)
{
    switch (err) {
    case NL_NOSERVICE:
	return "can't get service entry";
    case NL_NOHOST:
	return "can't get host entry";
    case NL_NOPROTO:
	return "can't get protocol entry";
    case NL_NOSOCK:
	return "can't create socket";
    case NL_NOSOCKOPT:
	return "error SETSOCKOPT SO_REUSEADDR";
    case NL_NOCONNECT:
	return "can't connect to host/port pair";
    default:
	return "unknown error";
    }
}

socket_t netlib_localsocket(const char *sockfile, int socktype)
/* acquire a connection to an existing Unix-domain socket */
{
#ifdef HAVE_SYS_UN_H
    int sock;

    if ((sock = socket(AF_UNIX, socktype, 0)) < 0) {
	return -1;
    } else {
	struct sockaddr_un saddr;

	memset(&saddr, 0, sizeof(struct sockaddr_un));
	saddr.sun_family = AF_UNIX;
	(void)strlcpy(saddr.sun_path,
		      sockfile,
		      sizeof(saddr.sun_path));

	if (connect(sock, (struct sockaddr *)&saddr, SUN_LEN(&saddr)) < 0) {
	    (void)close(sock);
	    return -2;
	}

	return sock;
    }
#else
    return -1;
#endif /* HAVE_SYS_UN_H */
}

char *netlib_sock2ip(socket_t fd)
/* retrieve the IP address corresponding to a socket */
{
    static char ip[INET6_ADDRSTRLEN];
#ifdef HAVE_INET_NTOP
    int r;
    sockaddr_t fsin;
    socklen_t alen = (socklen_t) sizeof(fsin);
    r = getpeername(fd, &(fsin.sa), &alen);
    if (r == 0) {
	switch (fsin.sa.sa_family) {
	case AF_INET:
	    r = !inet_ntop(fsin.sa_in.sin_family, &(fsin.sa_in.sin_addr),
			   ip, sizeof(ip));
	    break;
	case AF_INET6:
	    r = !inet_ntop((int)fsin.sa_in6.sin6_family,
                           &(fsin.sa_in6.sin6_addr), ip, sizeof(ip));
	    break;
	default:
	    (void)strlcpy(ip, "<unknown AF>", sizeof(ip));
	    return ip;
	}
    }
    /* Ugh... */
    if (r != 0)
#endif /* HAVE_INET_NTOP */
	(void)strlcpy(ip, "<unknown>", sizeof(ip));
    return ip;
}
