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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "interceptor.h"

#include <dlfcn.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "fd-assigner.h"
#include "fd2channel.h"
#include "req-resp-channel.h"
#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"

#undef INTERCEPTOR_RETURN__RET_AND_ERRNO
#define INTERCEPTOR_RETURN__RET_AND_ERRNO(callname)                                    \
    do {                                                                               \
        int ret = resp->callname->ret;                                                 \
        if (ret != -1) {                                                               \
            response__free_unpacked(resp, NULL);                                       \
            return ret;                                                                \
        }                                                                              \
                                                                                       \
        assert(resp->callname->has_errno_);                                            \
        /* response__free_unpacked 的时候有可能出错而导致重置 errno， */ \
        /*  我们不能现在就把 errno_ 赋值给 errno  */                        \
        int errno_ = resp->callname->errno_;                                           \
        response__free_unpacked(resp, NULL);                                           \
        errno = errno_;                                                                \
        return -1;                                                                     \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Glibc 提供的函数
//  供 interceptor 自己使用

struct glibc_socket_funcs glibc_funcs;

////////////////////////////////////////////////////////////////////////////////
// muQuinet 提供的实现

/* 1. Socket 创建/连接/关闭 */

int
socket(int domain, int type, int protocol)
{
    /*  1. 非 INET/TCP, INET/UDP 类型的 socket 创建请求
     *     放行给 glibc 来处理
     */

    if (domain != AF_INET) {
        // FIXME: logging
        printf("-- %s:%d: [INFO] pass [non-AF_INET] socket\n", __FILENAME__,
               __LINE__);
        return glibc_funcs.socket(domain, type, protocol);
    }

    if (!(type & SOCK_STREAM) && !(type & SOCK_DGRAM)) {
        // FIXME: logging
        printf("-- %s:%d: [INFO] pass [non-AF_INET/TCP, non-AF_INET/UDP] "
               "socket\n",
               __FILENAME__, __LINE__);
        return glibc_funcs.socket(domain, type, protocol);
    }

    /*  2. 请求 muQuinetd */

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] calling interceptor/socket {domain: AF_INET, "
               "type: %s}\n",
               __FILENAME__, __LINE__,
               (type & SOCK_STREAM) ? "SOCK_STREAM" : "SOCK_DGRAM");
    }

    Request req = REQUEST__INIT;
    Request__Socket sockCall = REQUEST__SOCKET__INIT;
    Response* resp;

    int connfd;
    channel connCh;

    {
        connfd = next_avail_fd();
        connCh = new_channel();
        set_fd2channel(connfd, connCh);

        {
            // FIXME: logging
            printf(
                "-- %s:%d: [INFO] assigned fd %d to conncection channel %lld\n",
                __FILENAME__, __LINE__, connfd, (long long)connCh);
        }
    }

    {
        req.pid = getpid();

        sockCall.domain = domain;
        sockCall.type = type;
        sockCall.protocol = protocol;

        req.socketcall = &sockCall;
        req.calling_case = REQUEST__CALLING_SOCKET_CALL;
    }

    {
        channel_send(connCh, &req);
        channel_recv(connCh, &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_SOCKET_CALL);

        int ret = resp->socketcall->ret;

        // happy path
        if (ret != -1) {
            response__free_unpacked(resp, NULL);
            return connfd;
        }

        // bad path
        assert(resp->socketcall->has_errno_);
        int errno_ = resp->socketcall->errno_;
        response__free_unpacked(resp, NULL);
        errno = errno_;
        return -1;
    }
}

int
connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.connect(sockfd, addr, addrlen);

    // AF_INET only, struct sockaddr_in only
    assert(addrlen == sizeof(struct sockaddr_in));

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] calling interceptor/connect {fd = %d, channel "
               "= %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd2channel(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Connect connCall = REQUEST__CONNECT__INIT;
    Response* resp;

    {
        req.pid = getpid();

        ProtobufCBinaryData addBytes = {.data = (uint8_t*)addr,
                                        .len = addrlen };
        connCall.addr = addBytes;
        req.connectcall = &connCall;
        req.calling_case = REQUEST__CALLING_CONNECT_CALL;
    }

    {
        channel_send(fd2channel(sockfd), &req);
        channel_recv(fd2channel(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_CONNECT_CALL);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(connectcall);
    }
}

int
close(int fd)
{
    if (!is_assigned_by_muquinet(fd))
        return glibc_funcs.close(fd);

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] calling interceptor/close {fd = %d, channel = "
               "%lld}\n",
               __FILENAME__, __LINE__, fd, (long long)fd2channel(fd));
    }

    Request req = REQUEST__INIT;
    Request__Close closeCall = REQUEST__CLOSE__INIT;
    Response* resp;

    {
        req.pid = getpid();
        req.closecall = &closeCall;
        req.calling_case = REQUEST__CALLING_CLOSE_CALL;
    }

    {
        channel_send(fd2channel(fd), &req);
        channel_recv(fd2channel(fd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_CLOSE_CALL);

        // close coresponding channel
        channel ch = fd2channel(fd);
        unset_fd2channel(fd, fd2channel(fd));
        close_channel(ch);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(closecall);
    }
}

/* 2. Socket reading/writing */

ssize_t
read(int fd, void* buf, size_t count)
{
    if (!is_assigned_by_muquinet(fd))
        return glibc_funcs.read(fd, buf, count);

    // forward to recvfrom
    return recvfrom(fd, buf, count, 0, NULL, NULL);
}

ssize_t
write(int fd, const void* buf, size_t count)
{
    if (!is_assigned_by_muquinet(fd))
        return glibc_funcs.write(fd, buf, count);

    // forward to sendto
    return sendto(fd, buf, count, 0, NULL, 0);
}

ssize_t
send(int sockfd, const void* buf, size_t len, int flags)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.send(sockfd, buf, len, flags);

    // forward to sendto
    return sendto(sockfd, buf, len, flags, NULL, 0);
}

ssize_t
sendto(int sockfd, const void* buf, size_t len, int flags,
       const struct sockaddr* dest_addr, socklen_t addrlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.sendto(sockfd, buf, len, flags, dest_addr, addrlen);

    if (!buf || !len) {
        return 0;
    }

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] calling interceptor/sendto {fd = %d, channel "
               "= %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd2channel(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Sendto sendtoCall = REQUEST__SENDTO__INIT;
    ProtobufCBinaryData bufData;
    ProtobufCBinaryData addrData;
    Response* resp;

    {
        req.pid = getpid();

        bufData.data = (uint8_t*)buf;
        bufData.len = len;

        sendtoCall.buf = bufData;
        sendtoCall.flags = flags;

        if (dest_addr) {
            addrData.data = (uint8_t*)dest_addr;
            assert(addrlen == sizeof(struct sockaddr_in));
            addrData.len = addrlen;
            sendtoCall.has_addr = true;
            sendtoCall.addr = addrData;
        }

        req.sendtocall = &sendtoCall;
        req.calling_case = REQUEST__CALLING_SENDTO_CALL;
    }

    {
        channel_send(fd2channel(sockfd), &req);
        channel_recv(fd2channel(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_SENDTO_CALL);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(recvfromcall);
    }
}

// sendmsg

ssize_t
recv(int sockfd, void* buf, size_t len, int flags)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.recv(sockfd, buf, len, flags);

    // forward to recvfrom
    return recvfrom(sockfd, buf, len, flags, NULL, NULL);
}

ssize_t
recvfrom(int sockfd, void* buf, size_t len, int flags,
         struct sockaddr* src_addr, socklen_t* restrict addrlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

    if (!buf || !len) {
        return 0;
    }

    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] calling interceptor/recvfrom {fd = %d, "
               "channel = %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd2channel(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Recvfrom recvfromCall = REQUEST__RECVFROM__INIT;
    Response* resp;

    {
        req.pid = getpid();

        recvfromCall.len = len;
        recvfromCall.flags = flags;
        if (src_addr) {
            recvfromCall.requireaddr = true;
        }

        req.recvfromcall = &recvfromCall;
        req.calling_case = REQUEST__CALLING_RECVFROM_CALL;
    }

    {
        channel_send(fd2channel(sockfd), &req);
        channel_recv(fd2channel(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_RECVFROM_CALL);

        // 有没有返回数据
        if (resp->recvfromcall->ret > 0) {
            ProtobufCBinaryData buf_bytes = resp->recvfromcall->buf;
            memcpy(buf, buf_bytes.data, buf_bytes.len);
        }

        // 要不要 & 有没有返回 peer 地址
        if (src_addr && resp->recvfromcall->ret >= 0) {
            assert(resp->recvfromcall->has_addr);
            // AF_INET only, sockaddr_in only
            ProtobufCBinaryData addr_bytes = resp->recvfromcall->addr;
            memcpy(src_addr, addr_bytes.data, sizeof(struct sockaddr_in));
            *addrlen = sizeof(struct sockaddr_in);
        }

        INTERCEPTOR_RETURN__RET_AND_ERRNO(recvfromcall);
    }
}

// recvmsg

/* 3. Socket polling */

int
poll(struct pollfd* fds, nfds_t nfds, int timeout)
{
    // TODO
}

int
select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
       struct timeval* timeout)
{
    // TODO
}

/* 4. Socket options */

int
getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.getpeername(sockfd, addr, addrlen);

    Request req = REQUEST__INIT;
    Request__Getpeername getpeernameCall = REQUEST__GETPEERNAME__INIT;
    Response* resp;

    {
        req.pid = getpid();

        req.getpeernamecall = &getpeernameCall;
        req.calling_case = REQUEST__CALLING_GETPEERNAME_CALL;
    }

    {
        channel_send(fd2channel(sockfd), &req);
        channel_recv(fd2channel(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_GETPEERNAME_CALL);
        ProtobufCBinaryData bufData = resp->recvfromcall->buf;
        memcpy(addr, bufData.data, bufData.len);
        *addrlen = sizeof(struct sockaddr_in);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(getpeernamecall);
    }
}

int
getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.getpeername(sockfd, addr, addrlen);

    Request req = REQUEST__INIT;
    Request__Getsockname getsocknameCall = REQUEST__GETSOCKNAME__INIT;
    Response* resp;

    {
        req.pid = getpid();

        req.getsocknamecall = &getsocknameCall;
        req.calling_case = REQUEST__CALLING_GETSOCKNAME_CALL;
    }

    {
        channel_send(fd2channel(sockfd), &req);
        channel_recv(fd2channel(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_GETSOCKNAME_CALL);
        ProtobufCBinaryData bufData = resp->recvfromcall->buf;
        memcpy(addr, bufData.data, bufData.len);
        *addrlen = sizeof(struct sockaddr_in);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(getpeernamecall);
    }
}

int
setsockopt(int sockfd, int level, int optname, const void* optval,
           socklen_t optlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.setsockopt(sockfd, level, optname, optval, optlen);

    // TODO
}

int
getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
    if (!is_assigned_by_muquinet(sockfd))
        return glibc_funcs.getsockopt(sockfd, level, optname, optval, optlen);

    // TODO
}

int
fcntl(int fd, int cmd, ...)
{
    if (!is_assigned_by_muquinet(fd)) {
        // TODO
    }

    // TODO
}

////////////////////////////////////////////////////////////////////////////////
// 拦截启动函数，令其初始化 interceptor

static void
interceptor_init()
{
    /* 1. create/connect/close */
    glibc_funcs.socket = dlsym(RTLD_NEXT, "socket");
    glibc_funcs.connect = dlsym(RTLD_NEXT, "connect");
    glibc_funcs.close = dlsym(RTLD_NEXT, "close");
    /* 2. read/write */
    glibc_funcs.read = dlsym(RTLD_NEXT, "read");
    glibc_funcs.write = dlsym(RTLD_NEXT, "write");
    glibc_funcs.send = dlsym(RTLD_NEXT, "send");
    glibc_funcs.sendto = dlsym(RTLD_NEXT, "sendto");
    glibc_funcs.recv = dlsym(RTLD_NEXT, "recv");
    glibc_funcs.recvfrom = dlsym(RTLD_NEXT, "recvfrom");
    /* 3. poll */
    glibc_funcs.poll = dlsym(RTLD_NEXT, "poll");
    glibc_funcs.select = dlsym(RTLD_NEXT, "select");
    /* 4. options */
    glibc_funcs.getpeername = dlsym(RTLD_NEXT, "getpeername");
    glibc_funcs.getsockname = dlsym(RTLD_NEXT, "getsockname");
    glibc_funcs.getsockopt = dlsym(RTLD_NEXT, "getsockopt");
    glibc_funcs.setsockopt = dlsym(RTLD_NEXT, "setsockopt");
    glibc_funcs.fcntl = dlsym(RTLD_NEXT, "fcntl");

    fd2channel_module_init();
}

int
__libc_start_main(int*(main)(int, char**, char**), int argc, char** ubp_av,
                  void (*init)(void), void (*fini)(void),
                  void (*rtld_fini)(void), void(*stack_end))
{
    interceptor_init();

    // 函数指针 glibc_start_main 声明
    int (*glibc_start_main)(int*(main)(int, char**, char**), int argc,
                            char** ubp_av, void (*init)(void),
                            void (*fini)(void), void (*rtld_fini)(void),
                            void(*stack_end)) = NULL;

    glibc_start_main = dlsym(RTLD_NEXT, "__libc_start_main");

    return glibc_start_main(main, argc, ubp_av, init, fini, rtld_fini,
                            stack_end);
}
