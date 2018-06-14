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

#ifndef MUQUINETD_MUX_REQRESPCHANNEL_H
#define MUQUINETD_MUX_REQRESPCHANNEL_H

#include <functional>
#include <memory>
#include <vector>

class Socket;
class Request;
class Response;
class SelectableChannel;

class ReqRespChannel : public std::enable_shared_from_this<ReqRespChannel>
{
public:
    // ReqRespChannel 拥有 fd，并负责在自己销毁时 close(fd)
    ReqRespChannel(int fd);
    ~ReqRespChannel();
    // Non-copyable, Non-moveable
    ReqRespChannel(const ReqRespChannel&) = delete;
    ReqRespChannel& operator=(const ReqRespChannel&) = delete;
    ReqRespChannel(ReqRespChannel&&) = delete;
    ReqRespChannel& operator=(ReqRespChannel&&) = delete;

    // 当 EventLoop 通知 ReqRespChannel read 事件时，ReqRespChannel
    // 自动调用 onNewRequest 所注册的回调函数来处理 Request
    //
    // 如上的整个自动处理过程位于 EventLoop IO 线程中，这要求注册的自动回调函数
    // onNewRequest **不能阻塞**
    //
    // 提示：自动回调函数 onNewRequest 内部可以以
    //  [ 告知 ProtocoControlBlock，让 ProtocolControlBlock 在资源可用时再
    //    主动 wirte(Response) ]
    //  这样的方式来避免阻塞
    //
    // Emmmm, 是否将 shared_ptr<ReqRespChannel> 作为 function
    //  的首参数是一个设计问题...
    //  这里，先不作为首参数，让 Mux 设置 lamba 的时候自己捕捉，传给
    //  RequestHandler.
    //
    // clang-format off
    void setOnNewRequestCB(
        const std::function
         <
           std::shared_ptr<Response>
           (const std::shared_ptr<ReqRespChannel>&
            , const std::shared_ptr<const Request>& req)
         > &
    );
    // clang-format on

    // 往 ReqRespChannel 中写 Response（主动读写，非回调路径）
    void write(const std::shared_ptr<Response>&);

    SelectableChannel* getSelectableChannel();

    // process information
    void setPeerName(const std::string&);
    const std::string& peerName();
    void setPeerPid(pid_t);

    // 下接 Socket
    std::shared_ptr<Socket> socket();
    void setSocket(const std::shared_ptr<Socket>&);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
