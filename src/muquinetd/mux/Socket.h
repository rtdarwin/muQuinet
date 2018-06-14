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

#ifndef MUQUINETD_MUX_SOCKET_H
#define MUQUINETD_MUX_SOCKET_H

#include <functional>
#include <memory>
#include <netinet/in.h>

class ReqRespChannel;
class SocketBuffer;
class SelectableChannel;
class Pcb;

class Socket
{
public:
    enum Type
    {
        UDP,
        TCP,
    };

public:
    Socket(Type, bool nonblocking);
    ~Socket();
    // Non-copyable, Non-moveable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(Socket&&) = delete;

    enum Type type();
    bool nonblocking();

    void setWaiting(bool);
    void setOnAsyncNewPacketCB(std::function<void()>);
    SelectableChannel* getAsyncNewPacketNotifyChannel();

    void putToRecvQ(struct sockaddr_in&, const std::shared_ptr<SocketBuffer>&);
    void takeFromRecvQ(struct sockaddr_in&, std::shared_ptr<SocketBuffer>&);
    bool try_takeFromRecvQ(struct sockaddr_in&, std::shared_ptr<SocketBuffer>&);

    // 上接 ReqRespChannel （其生命周期被 ReqRespChannel 管理）
    std::weak_ptr<ReqRespChannel> reqRespChannel();
    void setReqRespChannel(const std::shared_ptr<ReqRespChannel>&);

    // 下接 Pcb （管理 Pcb 的生命周期）
    std::shared_ptr<Pcb> pcb();
    void setPcb(const std::shared_ptr<Pcb>&);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
