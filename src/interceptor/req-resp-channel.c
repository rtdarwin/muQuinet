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

#include "req-resp-channel.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "interceptor.h"

#define WHILE_EINTR(expr)                                                      \
    while ((expr) == -1) {                                                     \
        if (errno == EINTR) {                                                  \
            continue;                                                          \
        } else {                                                               \
            perror(#expr);                                                     \
        }                                                                      \
    }

channel
new_channel()
{
    /* 1. 准备所需的 sockaddr_un 数据结构 */

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    const char* master_listener_path =
        RPC_MASTER_LISTENER_SOCKET_DIR "/" RPC_MASTER_LISTENER_SOCKET_NAME;
    strncpy(addr.sun_path, master_listener_path, sizeof(addr.sun_path) - 1);

    /* 2. 创建 socket 并 connect */

    int sockfd = glibc_funcs.socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int ret = glibc_funcs.connect(sockfd, (struct sockaddr*)&addr,
                                  sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] new req-resp-channel {fd = %d}\n",
               __FILENAME__, __LINE__, sockfd);
    }

    /* 3. return */

    return sockfd;
}

void
channel_send(channel ch, const Request* req)
{
    static __thread char buf[RPC_MESSAGE_MAX_SIZE];

    size_t bufsize = request__get_packed_size(req);
    assert(bufsize <= RPC_MESSAGE_MAX_SIZE);
    {
        // FIXME: logging
        printf("-- %s:%d: [DEBUG] Request bytes count = %ld\n", __FILENAME__,
               __LINE__, bufsize);
    }

    request__pack(req, (uint8_t*)buf);

    ssize_t nwritten = glibc_funcs.write(ch, buf, bufsize);
    {
        // FIXME: logging
        printf("-- %s:%d: [DEBUG] nwritten = %lld\n", __FILENAME__, __LINE__,
               (long long)nwritten);
    }

    assert((long long)nwritten == (long long)bufsize);
}

void
channel_recv(channel ch, Response** response)
{
    static __thread char buf[RPC_MESSAGE_MAX_SIZE];

    Response* resp = NULL;
    do {
        if (resp != NULL) {
            response__free_unpacked(resp, NULL);
            printf("-- %s:%d: [DEBUG] recv WAIT_NEXT\n", __FILENAME__,
                   __LINE__);
        }

        int nread;
        // clang-format off
        WHILE_EINTR
        (
            nread = glibc_funcs.read(ch, buf, RPC_MESSAGE_MAX_SIZE)
        );
        // clang-format on
        assert(nread != 0);

        {
            // FIXME: logging
            printf("-- %s:%d: [DEBUG] nread = %lld\n", __FILENAME__, __LINE__,
                   (long long)nread);
        }

        resp = response__unpack(NULL, nread, (uint8_t*)buf);

    } while (resp->retcode == RESPONSE__RET_CODE__WAIT_NEXT);

    *response = resp;
}

void
close_channel(channel ch)
{
    close(ch);
}
