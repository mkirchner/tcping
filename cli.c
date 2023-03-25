/*
 * tcping command line utility
 *
 * Copyright (c) 2002-2023 Marc Kirchner
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "tcping.h"

void usage(char *prog)
{
    fprintf(stderr,
            "error: Usage: %s [-q] [-f <4|6>] [-t timeout_sec] [-u "
            "timeout_usec] <host> <port>\n",
            prog);
    exit(-1);
}

int main(int argc, char *argv[])
{

    int force_ai_family = AF_UNSPEC;
    long timeout_sec = 0, timeout_usec = 0;
    struct timeval timeout;
    int verbosity = 1;

    if (argc < 3) {
        usage(argv[0]);
    }

    char *cptr;
    int c;
    while ((c = getopt(argc, argv, "qf:t:u:")) != -1) {
        switch (c) {
        case 'q':
            verbosity = 0;
            break;
        case 'f':
            cptr = NULL;
            long fam = strtol(optarg, &cptr, 10);
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

    timeout.tv_sec = timeout_sec + timeout_usec / 1000000;
    timeout.tv_usec = timeout_usec % 1000000;

    struct hostinfo *host;
    int err;
    int retval = 0;
    if ((err = tcping_gethostinfo(argv[optind], argv[optind + 1],
                                  force_ai_family, &host)) != 0) {
        log(verbosity, stderr, "error: %s\n", gai_strerror(err));
        retval = 255;
        goto quit;
    }
    int sockfd = tcping_socket(host);
    int result = tcping_connect(sockfd, host, &timeout);
    tcping_close(sockfd);
    switch (result) {
    case TCPING_ERROR:
        log(verbosity, stderr, "error: %s port %s: %s\n", host->name,
            host->serv, strerror(errno));
        retval = 255;
        break;
    case TCPING_OPEN:
        log(verbosity, stdout, "%s port %s open.\n", host->name, host->serv);
        break;
    case TCPING_CLOSED:
        log(verbosity, stdout, "%s port %s closed.\n", host->name, host->serv);
        retval = 1;
        break;
    case TCPING_TIMEOUT:
        log(verbosity, stdout, "%s port %s user timeout.\n", host->name,
            host->serv);
        retval = 2;
        break;
    default:
        log(verbosity, stderr, "error: invalid return value\n");
        retval = 255;
        break;
    }
quit:
    tcping_freehostinfo(host);
    return retval;
}
