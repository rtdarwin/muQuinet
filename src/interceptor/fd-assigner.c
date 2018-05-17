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

#include "fd-assigner.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fd2channel.h"
#include "interceptor.h"
#include "req-resp-channel.h"

extern int* g_fd2channelVector;

int g_startfd;
int g_fd_count;
// FIXME 回收利用已经 close 的 fd
static int last_fd_assigned;

static void reserve_fd_range();

void
ask_muquientd_fd_range()
{
    channel ch = get_proc_channel();
    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] asking muQuinetd file descriptors range...\n",
               __FILENAME__, __LINE__);
    }

    /*  1. 组装 request */

    Request req = REQUEST__INIT;
    Request__Atstart atstart = REQUEST__ATSTART__INIT;

    req.pid = getpid();

    atstart.progname = program_invocation_short_name;

    req.atstartaction = &atstart;
    req.calling_case = REQUEST__CALLING_ATSTART_ACTION;

    /*  2. 发送 request */

    channel_send(ch, &req);

    /*  3. 接收 response */

    Response* resp = NULL;
    channel_recv(ch, &resp);

    /*  4. 处理 response */

    g_startfd = resp->atstartaction->startfd;
    g_fd_count = resp->atstartaction->count;
    last_fd_assigned = g_startfd - 1;

    {
        // DEBUG
        assert(g_startfd != 0);
        assert(g_fd_count != 0);
        // FIXME logging
        printf("-- %s:%d: [INFO] fd range response from muQuientd: %d+%d\n",
               __FILENAME__, __LINE__, g_startfd, g_fd_count);
    }

    response__free_unpacked(resp, NULL);

    reserve_fd_range();
}

bool
is_assigned_by_muquinet(int fd)
{
    return (fd >= g_startfd && fd < g_startfd + g_fd_count);
}

int
next_avail_fd()
{
    // FIXME
    return last_fd_assigned + 1;
}

static void
reserve_fd_range()
{
    g_fd2channelVector = (int*)malloc(sizeof(int) * g_fd_count);
    // TODO
}
