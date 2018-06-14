#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int
main(int argc, char* argv[])
{
    char* dest_ipaddr = argv[1];
    short dest_port = atoi(argv[2]);

    /* 1. socket */

    {
        printf("socket\n");
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("sockfd");
    }

    /* 2. conncect */

    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    inet_pton(AF_INET, dest_ipaddr, &remote.sin_addr);
    remote.sin_port = htons(dest_port);

    {
        printf("connect\n");
    }
    int ret = connect(sockfd, &remote, sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("conncect");
    }

    /* 3. sockanme, peername */

    struct sockaddr_in sockname;
    struct sockaddr_in peername;
    char ipaddrp[INET_ADDRSTRLEN + 1] = { 0 };

    socklen_t len;
    getsockname(sockfd, &sockname, &len);
    getpeername(sockfd, &peername, &len);

    {
        inet_ntop(AF_INET, &sockname.sin_addr, ipaddrp, INET_ADDRSTRLEN);
        printf("sockname = {addr = %s, port = %d}\n", ipaddrp,
               ntohs(sockname.sin_port));

        inet_ntop(AF_INET, &peername.sin_addr, ipaddrp, INET_ADDRSTRLEN);
        printf("peername = {addr = %s, port = %d}\n", ipaddrp,
               ntohs(peername.sin_port));
    }

    /* 4. sendto */

    const char* msg = "hello, world!";

    {
        printf("send: %s\n", msg);
    }
    send(sockfd, msg, 13, 0);

    /* 5. recvfrom */

    {
        printf("recv\n");
    }
    char buf[256];
    recv(sockfd, buf, 256, 0);
    {
        printf("recv: %s\n", buf);
    }

    /* - close */

    // TODO

    return 0;
}
