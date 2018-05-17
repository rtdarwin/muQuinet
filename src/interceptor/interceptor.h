/*
 * muQuinet, an userspace TCP/IP network stack.
 * Copyright (C) 2018 rtdarwin
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef MUQUINET_INTERCEPTOR_H
#define MUQUINET_INTERCEPTOR_H

#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define __FILENAME__                                                           \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

struct glibc_socket_funcs
{
    /* 1. create/connect/close */

    int (*socket)(int domain, int type, int protocol);
    int (*connect)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    int (*close)(int fd);

    /* 2. read/write */

    ssize_t (*read)(int fd, void* buf, size_t count);
    ssize_t (*write)(int fd, const void* buf, size_t count);
    ssize_t (*send)(int sockfd, const void* buf, size_t len, int flags);
    ssize_t (*sendto)(int sockfd, const void* buf, size_t len, int flags,
                      const struct sockaddr* dest_addr, socklen_t addrlen);
    // sendmsg
    ssize_t (*recv)(int sockfd, void* buf, size_t len, int flags);
    ssize_t (*recvfrom)(int sockfd, void* buf, size_t len, int flags,
                        struct sockaddr* src_addr, socklen_t* addrlen);
    // recvmsg

    /* 3. poll */

    int (*poll)(struct pollfd* fds, nfds_t nfds, int timeout);
    int (*select)(int nfds, fd_set* readfds, fd_set* writefds,
                  fd_set* exceptfds, struct timeval* timeout);

    /* 4. options */

    int (*getpeername)(int socketfd, struct sockaddr* addr, socklen_t* addrlen);
    int (*getsockname)(int socketfd, struct sockaddr* addr, socklen_t* addlen);
    int (*getsockopt)(int fd, int level, int optname, const void* optval,
                      socklen_t* optlen);
    int (*setsockopt)(int fd, int level, int optname, const void* optval,
                      socklen_t optlen);
    int (*fcntl)(int fd, int cmd, ...);
};

extern struct glibc_socket_funcs glibc_funcs;

#endif
