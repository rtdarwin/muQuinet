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

#ifndef INTERCEPTOR_REQRESPCHANNEL_H
#define INTERCEPTOR_REQRESPCHANNEL_H

#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"
#include "rpc/rpc.h"

typedef int channel;

channel new_channel();
void channel_send(channel ch, const Request* req);
// 由于 unpack 的时候 protobuf-c 自己帮你创建 Response,
// 这里只能用参数 Response** 来将 protobuf-c 创建的 Response 返回，
// 而不能用参数 Response* 来更改调用者的 Response
void channel_recv(channel ch, Response** resp);
void close_channel(channel);

#endif
