/*
 * tcping.c
 *
 * Copyright (c) 2002-2023 Marc Kirchner
 *
 * tcping does a nonblocking connect to test if a port is reachable.
 * Its exit codes are:
 *     -1 an error occured
 *     0  port is open
 *     1  port is closed
 *     2  user timeout
 */

#define VERSION 2.0.0

#define log(verbosity, fmt, ...)                                               \
    do {                                                                       \
        if (verbosity)                                                         \
            fprintf(fmt, __VA_ARGS__);                                         \
    } while (0)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void usage();

int main(int argc, char *argv[])
{

    char *cptr;
    struct addrinfo hints, *res;
    int c;
    int err;
    socklen_t errlen;
    int error = 0;
    long fam;
    fd_set fdrset, fdwset;
    int force_ai_family = AF_UNSPEC;
    char hostname[INET6_ADDRSTRLEN];
    int ret;
    char servicename[6];
    int sockfd;
    long timeout_sec = 0, timeout_usec = 0;
    struct timeval timeout;
    int verbosity = 1;

    if (argc < 3) {
        usage(argv[0]);
    }

    while ((c = getopt(argc, argv, "qf:t:u:")) != -1) {
        switch (c) {
        case 'q':
            verbosity = 0;
            break;
        case 'f':
            cptr = NULL;
            fam = strtol(optarg, &cptr, 10);
            if (cptr == optarg || (fam != 4 && fam != 6))
                usage(argv[0]);
            force_ai_family = fam == 4 ? AF_INET : AF_INET6;
            break;
        case 't':
            cptr = NULL;
            timeout_sec = strtol(optarg, &cptr, 10);
            if (cptr == optarg)
                usage(argv[0]);
            break;
        case 'u':
            cptr = NULL;
            timeout_usec = strtol(optarg, &cptr, 10);
            if (cptr == optarg)
                usage(argv[0]);
            break;
        default:
            usage(argv[0]);
            break;
        }
    }
    if (!argv[optind + 1]) {
        usage(argv[0]);
    }

    /* collect all ipv4 and ipv6 address info */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = force_ai_family;
    hints.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(argv[optind], argv[optind + 1], &hints, &res)) !=
        0) {
        log(verbosity, stderr, "error: %s\n", gai_strerror(err));
        exit(-1);
    }
    if ((err = getnameinfo(res->ai_addr, res->ai_addrlen, hostname,
                           sizeof(hostname), servicename, sizeof(servicename),
                           NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
        log(verbosity, stderr, "error: %s\n", gai_strerror(err));
        exit(-1);
    }
    /* we only look at the first resolution result */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    /* attempt the connection */
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (ret) {
        if (errno != EINPROGRESS) {
            log(verbosity, stderr, "error: %s port %s: %s\n", hostname,
                servicename, strerror(errno));
            exit(-1);
        }

        FD_ZERO(&fdrset);
        FD_SET(sockfd, &fdrset);
        fdwset = fdrset;

        timeout.tv_sec = timeout_sec + timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;

        if ((ret = select(sockfd + 1, &fdrset, &fdwset, NULL,
                          timeout.tv_sec + timeout.tv_usec > 0 ? &timeout
                                                               : NULL)) == 0) {
            /* timeout */
            close(sockfd);
            log(verbosity, stdout, "%s port %s user timeout.\n", hostname,
                servicename);
            return 2;
        }
        if (FD_ISSET(sockfd, &fdrset) || FD_ISSET(sockfd, &fdwset)) {
            errlen = sizeof(error);
            if ((ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error,
                                  &errlen)) != 0) {
                /* getsockopt error */
                log(verbosity, stderr, "error: %s port %s: getsockopt: %s\n",
                    hostname, servicename, strerror(errno));
                close(sockfd);
                exit(-1);
            }
            if (error != 0) {
                log(verbosity, stdout, "%s port %s closed.\n", hostname,
                    servicename);
                close(sockfd);
                return 1;
            }
        } else {
            log(verbosity, stderr, "error: select: sockfd not set\n");
            exit(-1);
        }
    }
    /* OK, connection established */
    close(sockfd);
    log(verbosity, stdout, "%s port %s open.\n", hostname, servicename);
    return 0;
}

void usage(char *prog)
{
    fprintf(stderr,
            "error: Usage: %s [-q] [-f <4|6>] [-t timeout_sec] [-u "
            "timeout_usec] <host> "
            "<port>\n",
            prog);
    exit(-1);
}
