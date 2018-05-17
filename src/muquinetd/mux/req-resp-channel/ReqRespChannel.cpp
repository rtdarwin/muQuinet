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

#include "ReqRespChannel.h"

#include <google/protobuf/util/json_util.h>
#include <string>
#include <sys/un.h>
#include <unistd.h>

#include "muquinetd/Logging.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "rpc/cpp_out/request.pb.h"
#include "rpc/cpp_out/response.pb.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::string;
using std::vector;

class Connection;

struct ReqRespChannel::Impl
{
    /*  Data members */

    int fd;
    // TODO: multiprocess support
    string peer;            // Name of peer interceptor
    vector<pid_t> peerPids; // Pids of peer interceptors
    bool closeOnExec = false;
    int refcnt = 1; // How many interceptors are referening this channel
    unique_ptr<SelectableChannel> sChannel; // For EventLoop use
    shared_ptr<Socket> socket;

    /*  Member functions */

    // clang-format off
    std::function
     <
        shared_ptr<Response>
        (const shared_ptr<ReqRespChannel>& rrChannel
         , const shared_ptr<const Request>& req)
     > onNewRequestFunc;
    // clang-format on
    void onReadReady(const shared_ptr<ReqRespChannel>&);
    void write(const shared_ptr<Response>&);
};

ReqRespChannel::ReqRespChannel(int fd)
{
    {
        MUQUINETD_LOG(debug) << "New ReqRespChannel {fd = " << fd << "}";
    }

    _pImpl.reset(new ReqRespChannel::Impl);
    _pImpl->fd = fd;

    /*  1. SelectableChannel for EventLoop */

    auto& sChannel = _pImpl->sChannel;
    sChannel.reset(new SelectableChannel(fd));
    sChannel->enableReading();
    sChannel->onRead(
        [this]() { this->_pImpl->onReadReady(this->shared_from_this()); });
}

ReqRespChannel::~ReqRespChannel()
{
    ::close(_pImpl->fd);

    {
        MUQUINETD_LOG(debug) << "Destroy ReqRespChannel";
    }
}

void
ReqRespChannel::onNewRequest(const std::function<shared_ptr<Response>(
                                 const shared_ptr<ReqRespChannel>&,
                                 const shared_ptr<const Request>& req)>& f)
{
    assert(f);
    _pImpl->onNewRequestFunc = f;
}

void
ReqRespChannel::write(const std::shared_ptr<Response>& resp)
{
    _pImpl->write(resp);
}

SelectableChannel*
ReqRespChannel::getSelectableChannel()
{
    return _pImpl->sChannel.get();
}

void
ReqRespChannel::setPeerName(const std::string& p)
{
    _pImpl->peer = p;
}

const std::string&
ReqRespChannel::peerName()
{
    return _pImpl->peer;
}

void
ReqRespChannel::addPeerPid(pid_t p)
{
    _pImpl->peerPids.push_back(p);
}

const std::vector<pid_t>&
ReqRespChannel::peerPids()
{
    return _pImpl->peerPids;
}

void
ReqRespChannel::setCloseOnExec(bool cloexec)
{
    _pImpl->closeOnExec = cloexec;
}

bool
ReqRespChannel::closeOnExec()
{
    return _pImpl->closeOnExec;
}

void
ReqRespChannel::increaseRefCount()
{
    _pImpl->refcnt++;
}

void
ReqRespChannel::decreaseRefCount()
{
    _pImpl->refcnt--;
}

int
ReqRespChannel::refCount()
{
    return _pImpl->refcnt;
}

shared_ptr<Socket>
ReqRespChannel::socket()
{
    return _pImpl->socket;
}

void
ReqRespChannel::setSocket(const std::shared_ptr<Socket>& s)
{
    _pImpl->socket = s;
}

void
ReqRespChannel::Impl::onReadReady(const shared_ptr<ReqRespChannel>& rrChannel)
{
    // 每线程一个 rdbuf (从 unix socket 读取数据)
    // （这么做可能有点违反对象编程直觉，但可以极大减少存储空间消耗）
    static __thread char rdbuf[RPC_MESSAGE_MAX_SIZE];

    // 防止本函数运行过程中 rrChannel 被销毁（不这么做的话，确实会）
    shared_ptr<ReqRespChannel> holdit{ rrChannel };

    shared_ptr<Request> req = make_shared<Request>();
    shared_ptr<Response> resp;

    /*  1. read the message */

    memset(rdbuf, 0, sizeof(rdbuf));
    int nread = ::read(fd, rdbuf, sizeof(rdbuf));
    if (nread == -1) {
        // FIXME: EAGAIN, EWOULDBLOCK
        int errno_ = errno;
        MUQUINETD_LOG(error) << "::read " << string(strerror(errno_))
                             << ". This channel will be closed";
        sChannel->unregisterSelf();
        return;
    } else if (nread == 0) {
        MUQUINETD_LOG(error) << "::read EOF"
                             << ". This channel will be closed";
        sChannel->unregisterSelf();
        return;
    } else if (nread == RPC_MESSAGE_MAX_SIZE) {
        MUQUINETD_LOG(warning) << "::read call get RPC_MESSAGE_MAX_SIZE bytes";
    }

    /*  2. parse the message */

    req->ParseFromArray(rdbuf, nread);

    {
        // For DEBUG
        string jsonStr;
        google::protobuf::util::MessageToJsonString(*req, &jsonStr);
        MUQUINETD_LOG(debug)
            << "Recv " << nread << " bytes, request message: " << jsonStr;
    }

    /*  3. handle the request message, generate response message */

    assert(onNewRequestFunc);
    resp = onNewRequestFunc(rrChannel, req);

    /*  4. write the message */

    write(resp);
}

void
ReqRespChannel::Impl::write(const std::shared_ptr<Response>& resp)
{
    static __thread char wrbuf[RPC_MESSAGE_MAX_SIZE];

    {
        // For DEBUG
        string jsonStr;
        google::protobuf::util::MessageToJsonString(*resp.get(), &jsonStr);
        MUQUINETD_LOG(debug) << "Send response message: " << jsonStr;
    }

    assert(resp->IsInitialized());
    int needwr = resp->ByteSize();
    assert(needwr <= RPC_MESSAGE_MAX_SIZE);
    // if (needwr > RPC_MESSAGE_MAX_SIZE) {
    //     MUQUINETD_LOG(error)
    //         << "Serialized message size exceeds RPC_MESSAGE_MAX_SIZE";
    // }
    resp->SerializeToArray(wrbuf, RPC_MESSAGE_MAX_SIZE);
    int nwritten = ::write(fd, wrbuf, needwr);
    if (nwritten == -1) {
        int errno_ = errno;
        MUQUINETD_LOG(error) << "::write " << string(strerror(errno_))
                             << ". This channel will be closed";
        sChannel->unregisterSelf();
        return;
    } else if (nwritten < needwr) {
        MUQUINETD_LOG(error)
            << "::write <<" << nwritten << " bytes, which less than" << needwr
            << " bytes to write";
        // FIXME: just like shadowsocks-libev, use a channel for write event ?
    }
}
