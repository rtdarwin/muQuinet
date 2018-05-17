#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int
main(int argc, char* argv[])
{
    /* 1. socket */

    {
        printf("socket\n");
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("sockfd");
    }

    /* 2. sendto */

    const char* msg = "hello, world!\n";
    struct sockaddr_in remote;
    bzero(&remote, sizeof(struct sockaddr_in));

    remote.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.168.8", &remote.sin_addr);
    remote.sin_port = htons(80);

    {
        printf("sendto: %s\n", msg);
    }
    sendto(sockfd, msg, 14, 0, &remote, sizeof(struct sockaddr_in));

    /* 3. sockanme, peername */

    struct sockaddr_in sockname;
    struct sockaddr_in peername;
    bzero(&sockname, sizeof(struct sockaddr_in));
    bzero(&sockname, sizeof(struct sockaddr_in));
    char ipaddrp[INET_ADDRSTRLEN + 1] = { 0 };

    socklen_t len;
    {
        printf("getsockname\n");
    }
    getsockname(sockfd, &sockname, &len);
    {
        printf("getpeername\n");
    }
    getpeername(sockfd, &peername, &len);

    {
        inet_ntop(AF_INET, &sockname.sin_addr, ipaddrp, INET_ADDRSTRLEN);
        printf("sockname = {addr = %s, port = %d}\n", ipaddrp,
               ntohs(sockname.sin_port));

        inet_ntop(AF_INET, &peername.sin_addr, ipaddrp, INET_ADDRSTRLEN);
        printf("peername = {addr = %s, port = %d}\n", ipaddrp,
               ntohs(peername.sin_port));
    }

    /* 4. recvfrom */

    char recv_buf[256] = { 0 };

    {
        printf("recvfrom\n");
    }
    recvfrom(sockfd, recv_buf, 255, 0, &remote, &len);

    {
        bzero(ipaddrp, sizeof(ipaddrp));
        inet_ntop(AF_INET, &remote.sin_addr, ipaddrp, INET_ADDRSTRLEN);
        printf("recvfrom {ip = %s, port = %d}: %s\n", ipaddrp,
               ntohs(remote.sin_port), recv_buf);
    }

    /* - close */

    close(sockfd);

    return 0;
}
