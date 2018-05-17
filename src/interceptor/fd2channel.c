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

#include "fd2channel.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "fd-assigner.h"
#include "interceptor.h"
#include "req-resp-channel.h"

int* g_fd2channelVector;
extern int g_startfd;
extern int g_fd_count;

static channel g_proc_channel;

void
fd2channel_module_init()
{
    // process 级 channel
    channel ch = new_channel();
    g_proc_channel = ch;
    {
        // FIXME: logging
        printf("-- %s:%d: [INFO] create process channel {channel = %lld}\n",
               __FILENAME__, __LINE__, (long long)ch);
    }

    // reserve fd range
    ask_muquientd_fd_range();
}

channel
fd2channel(int fd)
{
    return g_fd2channelVector[fd - g_startfd];
}

void
set_fd2channel(int fd, channel ch)
{
    g_fd2channelVector[fd - g_startfd] = ch;
}

void
unset_fd2channel(int fd, channel ch)
{
    g_fd2channelVector[fd - g_startfd] = 0;
}

channel
get_proc_channel()
{
    return g_proc_channel;
}
