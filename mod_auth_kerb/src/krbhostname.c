#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

int main(int argc, char **argv)
{
    struct addrinfo *ai, hints;
    char *hostname;
    char localname[MAXHOSTNAMELEN];
    char hnamebuf[NI_MAXHOST];

    if(gethostname(localname, MAXHOSTNAMELEN)) {
        return 1;
    }
    printf("gethostname() returns \"%s\"\n", localname);

    if(argc == 2) {
        hostname = argv[1];
    } else {
        hostname = localname;
    }
    printf("Using \"%s\" as service host name\n", hostname);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if(getaddrinfo(hostname, NULL, &hints, &ai)) {
        printf("getaddrinfo() failed\n");
        return 1;
    }

    if(ai->ai_canonname) {
        hostname = strdup(ai->ai_canonname);
        printf("Canonical hostname from getaddrinfo() is \"%s\"\n", hostname);
    }
    
    if(getnameinfo(ai->ai_addr, ai->ai_addrlen, hnamebuf, sizeof(hnamebuf),
        NULL, 0, NI_NAMEREQD)) {
        printf("getnameinfo() failed\n");
        return 1;
    }

    printf("Hostname from getnameinfo() is \"%s\"\n", hnamebuf);

    freeaddrinfo(ai);
    return 0;
}
