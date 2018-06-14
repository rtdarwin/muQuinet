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

#include "Socket.h"

#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

#include <functional>
#include <utility>

#include "muquinetd/Logging.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "muquinetd/mux/req-resp-channel/ReqRespChannel.h"
#include "third-party/concurrentqueue/blockingconcurrentqueue.h"

class SocketBuffer;

using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using moodycamel::BlockingConcurrentQueue;

struct Socket::Impl
{
    enum Socket::Type type;

    bool nonblocking;
    int new_packet_notify_pipe[2];
    bool waiting = false;
    std::function<void()> onAsyncNewPacket;
    unique_ptr<SelectableChannel> sChannel;

    std::weak_ptr<ReqRespChannel> rrChannel;
    std::shared_ptr<Pcb> pcb;

    typedef BlockingConcurrentQueue<
        std::pair<struct sockaddr_in, shared_ptr<SocketBuffer>>>
        RecvQType;
    RecvQType recvQ;
    static const int recvQlimit = 128;

public:
    void markSChannelAsReadable();
    void sChannelReadCb();
};

Socket::Socket(enum Type t, bool isnonblocking)
{
    {
        MUQUINETD_LOG(debug) << "New Socket";
    }

    _pImpl.reset(new Socket::Impl);

    _pImpl->type = t;
    _pImpl->nonblocking = isnonblocking;

    // SelectableChannel
    pipe2(_pImpl->new_packet_notify_pipe, O_NONBLOCK);

    _pImpl->sChannel.reset(
        new SelectableChannel(_pImpl->new_packet_notify_pipe[0]));
    _pImpl->sChannel->enableReading();
    _pImpl->sChannel->setOnReadCB(
        std::bind(&Socket::Impl::sChannelReadCb, this->_pImpl.get()));
}

Socket::~Socket()
{
    MUQUINETD_LOG(debug) << "Destroy Socket";
}

enum Socket::Type
Socket::type()
{
    return _pImpl->type;
}

bool
Socket::nonblocking()
{
    return _pImpl->nonblocking;
}

SelectableChannel*
Socket::getAsyncNewPacketNotifyChannel()
{
    assert(_pImpl->sChannel);

    return _pImpl->sChannel.get();
}

void
Socket::setWaiting(bool v)
{
    _pImpl->waiting = v;

    if (v == true) {
        MUQUINETD_LOG(info) << "Socket waiting state set to true, Socket will "
                               "delegate EventLoop to wait for new async "
                               "packets arrive";
    } else {
        MUQUINETD_LOG(info) << "Socket waiting state set to false, EventLoop "
                               "will not wait for new async arrives any more";
    }
}

void
Socket::setOnAsyncNewPacketCB(std::function<void()> cb)
{
    assert(cb);
    _pImpl->onAsyncNewPacket = cb;
}

void
Socket::putToRecvQ(struct sockaddr_in& peeraddr,
                   const std::shared_ptr<SocketBuffer>& skbuf)
{
    auto& q = _pImpl->recvQ;

    if (q.size_approx() > Socket::Impl::recvQlimit)
        return;

    q.enqueue(std::make_pair(peeraddr, skbuf));
    _pImpl->markSChannelAsReadable();
}

void
Socket::takeFromRecvQ(struct sockaddr_in& addr,
                      std::shared_ptr<SocketBuffer>& skbuf)
{
    auto pairToPopulate = std::make_pair(std::ref(addr), std::ref(skbuf));

    _pImpl->recvQ.wait_dequeue(pairToPopulate);
}

bool
Socket::try_takeFromRecvQ(struct sockaddr_in& addr,
                          std::shared_ptr<SocketBuffer>& skbuf)
{
    auto pairToPopulate = std::make_pair(std::ref(addr), std::ref(skbuf));

    return _pImpl->recvQ.try_dequeue(pairToPopulate);
}

weak_ptr<ReqRespChannel>
Socket::reqRespChannel()
{
    return _pImpl->rrChannel;
}

void
Socket::setReqRespChannel(const std::shared_ptr<ReqRespChannel>& ch)
{
    assert(ch);
    _pImpl->rrChannel = ch;
}

shared_ptr<Pcb>
Socket::pcb()
{
    return _pImpl->pcb;
}

void
Socket::setPcb(const std::shared_ptr<Pcb>& p)
{
    assert(p);
    _pImpl->pcb = p;
}

void
Socket::Impl::markSChannelAsReadable()
{
    write(new_packet_notify_pipe[1], "1", 1);
}

void
Socket::Impl::sChannelReadCb()
{
    // read until -1 and EAGAIN
    char readbuf[8];
    while (::read(new_packet_notify_pipe[0], readbuf, 8) != -1)
        ;

    if (!this->waiting) {
        return;
    }

    {
        MUQUINETD_LOG(info) << "New async packet arrives, Socket will pass it "
                               "to processes waitting for it";
    }

    if (onAsyncNewPacket) {
        onAsyncNewPacket();
    }

    this->waiting = false;
}
