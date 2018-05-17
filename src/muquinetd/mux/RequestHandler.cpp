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

#include "RequestHandler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "muquinetd/Logging.h"
#include "muquinetd/Mux.h"
#include "muquinetd/Pcb.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/Tcp.h"
#include "muquinetd/Udp.h"
#include "muquinetd/mux/EventLoop.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/mux/req-resp-channel/ReqRespChannel.h"
#include "rpc/cpp_out/request.pb.h"
#include "rpc/cpp_out/response.pb.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

shared_ptr<Response>
RequestHandler::handleRequest(const shared_ptr<ReqRespChannel>& rrChannel,
                              const shared_ptr<const Request>& req)
{
    auto resp = make_shared<Response>();

    switch (req->calling_case()) {
        case Request::kSocketCall: // DONE
            MUQUINETD_LOG(info) << "Handling socketCall...";
            socketCall(rrChannel, req, resp);
            break;
        case Request::kConnectCall: // TODO (TCP)
            MUQUINETD_LOG(info) << "Handling conncectCall...";
            connectCall(rrChannel, req, resp);
            break;
        case Request::kCloseCall: // TODO
            MUQUINETD_LOG(info) << "Handling closeCall...";
            closeCall(rrChannel, req, resp);
            break;
        case Request::kRecvfromCall: // TODO
            MUQUINETD_LOG(info) << "Handling recvfromCall...";
            recvfromCall(rrChannel, req, resp);
            break;
        case Request::kSendtoCall: // TODO
            MUQUINETD_LOG(info) << "Handling sendtoCall...";
            sendtoCall(rrChannel, req, resp);
            break;
        case Request::kPollCall: // TODO
            break;
        case Request::kSelectCall: // TODO
            break;
        case Request::kGetpeernameCall:
            MUQUINETD_LOG(info) << "Handling getpeernameCall...";
            getpeernameCall(rrChannel, req, resp);
            break;
        case Request::kGetsocknameCall:
            MUQUINETD_LOG(info) << "Handlinig getsocknameCall...";
            getsocknameCall(rrChannel, req, resp);
            break;
        case Request::kGetsockoptCall: // TODO
            break;
        case Request::kSetsockoptCall: // TODO
            break;
        case Request::kFcntlCall: // TODO
            break;

        case Request::kAtstartAction:
            MUQUINETD_LOG(info) << "Handling atstartAction...";
            atstartAction(rrChannel, req, resp);
            break;
        case Request::kAtforkAction: // TODO
            break;
        case Request::kAtexitAction: // TODO
            break;

        case Request::CALLING_NOT_SET: // TODO
            MUQUINETD_LOG(warning) << "Request is empty";
            break;
        default:
            MUQUINETD_LOG(warning) << "Unknown Request type";
            break;
    }

    return resp;
}

void
RequestHandler::socketCall(const shared_ptr<ReqRespChannel>& rrChannel,
                           const shared_ptr<const Request>& req,
                           const shared_ptr<Response>& resp)
{
    const auto& call = req->socketcall();
    int32_t domain = call.domain();
    int32_t type = call.type();
    // int32_t protocol = call.protocol(); // Ignore it

    assert(domain == AF_INET);
    assert(type & SOCK_STREAM || type & SOCK_DGRAM);

    // TODO: multiple process support: pid_t pid

    /*  1. 创建 Socket, Pcb */

    shared_ptr<Socket> so;
    shared_ptr<Pcb> pcb;

    bool nonblocking = type & SOCK_NONBLOCK;
    bool cloexec = type & SOCK_CLOEXEC;

    rrChannel->setCloseOnExec(cloexec);

    if (type & SOCK_STREAM) {
        so = make_shared<Socket>(Socket::Type::TCP, nonblocking);
        pcb = Tcp::get()->newPcb();
    } else if (type & SOCK_DGRAM) {
        so = make_shared<Socket>(Socket::Type::UDP, nonblocking);
        pcb = Udp::get()->newPcb();
    }

    /*  2. 让 RRChannel, Socket, Pcb 相互指向 */

    pcb->setSocket(so);
    so->setPcb(pcb);
    so->setReqRespChannel(rrChannel);
    rrChannel->setSocket(so);

    /*  3. 把 Socket.selectableChannel 加入 EventLoop */

    SelectableChannel* sChannel = so->getSelectableChannel();
    EventLoop* eventloop = Mux::get()->eventLoop();
    eventloop->addChannel(sChannel);
    sChannel->setOwnerEventLoop(eventloop);

    /*  4. 返回信息给 Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_socketcall();
    callRet->set_ret(0);
}

void
RequestHandler::connectCall(const shared_ptr<ReqRespChannel>& rrChannel,
                            const shared_ptr<const Request>& req,
                            const shared_ptr<Response>& resp)
{
    const std::string& destAddr_str = req->connectcall().addr();
    const char* destAddr_bytes = destAddr_str.c_str();
    sockaddr_in destAddr;
    memcpy(&destAddr, destAddr_bytes, sizeof(struct sockaddr_in));

    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();

    // TODO
    //
    // - ECONNREFUSED: A connect() on a stream socket found no one
    //                 listening on the remote address. (TCP only)
    // - EINPROGRESS: The socket is nonblocking and the connection
    //                cannot be completed immediately. (TCP only)
    // - ETIMEDOUT: Timeout while attempting connection. (TCP only)

    /*  1. 转交 Pcb 处理 */

    int ret = pcb->connect(destAddr.sin_addr, destAddr.sin_port);

    /*  2. 返回信息给 Interceptor */

    auto* callRet = resp->mutable_connectcall();

    if (ret == 0) {
        callRet->set_ret(0);
    } else {
        callRet->set_ret(-1);
        callRet->set_errno_(ret);
    }
}

void
RequestHandler::closeCall(const shared_ptr<ReqRespChannel>& rrChannel,
                          const shared_ptr<const Request>& req,
                          const shared_ptr<Response>& resp)
{
    /*  1. 减少引用，引用减少为 0 时销毁 */

    rrChannel->decreaseRefCount();
    if (!rrChannel->refCount()) {
        Mux::get()->removeRRChannel(rrChannel);
    }

    /*  2. 返回信息给 Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_closecall();
    callRet->set_ret(0);
}

void
RequestHandler::sendtoCall(const shared_ptr<ReqRespChannel>& rrChannel,
                           const shared_ptr<const Request>& req,
                           const shared_ptr<Response>& resp)
{
    const auto& call = req->sendtocall();
    const std::string& buf = call.buf();
    // int32_t flags = call.flags(); // unused
    bool hasAddr = call.has_addr();

    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    int nwritten = 0;

    {
        if (socket->type() == Socket::Type::UDP) {
            MUQUINETD_LOG(info) << "It's an UDP send request, passing it to "
                                   "corresponding UdpPcb...";
            if (hasAddr) {
                const char* destAddr_bytes = call.addr().c_str();
                struct sockaddr_in destAddr;
                bzero(&destAddr, sizeof(struct sockaddr_in));
                memcpy(&destAddr, destAddr_bytes, sizeof(struct sockaddr_in));

                nwritten = pcb->send(destAddr.sin_addr, destAddr.sin_port, buf);
            } else {
                nwritten = pcb->send(buf);
            }
        } else if (socket->type() == Socket::Type::TCP) {
            MUQUINETD_LOG(info) << "It's a TCP send request, passing it to "
                                   "corresponding TcpPcb...";
            nwritten = pcb->send(buf);
        }
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_sendtocall();
        callRet->set_ret(nwritten);
    }
}

void
RequestHandler::recvfromCall(const shared_ptr<ReqRespChannel>& rrChannel,
                             const shared_ptr<const Request>& req,
                             const shared_ptr<Response>& resp)
{
    // TODO: TCP

    const auto& call = req->recvfromcall();
    const auto& socket = rrChannel->socket();
    // int32_t flags = call.flags(); // ignored
    bool require_addr = call.requireaddr();

    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(sockaddr_in));
    shared_ptr<SocketBuffer> skbuf_head;

    {
        if (socket->nonblocking()) {
            if (socket->try_takeFromRecvQ(peeraddr, skbuf_head)) {
                // happy path
                // fall through
            } else {
                // bad path
                goto e_again;
            }
        } else {
            // FIXME: this function call will block eventloop thread
            if (socket->try_takeFromRecvQ(peeraddr, skbuf_head)) {
                // happy path
                // fall through
            } else {
                // bad path
                goto wait_next;
            }
        }
    }

    // happy path
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_recvfromcall();
        std::string* buf = callRet->mutable_buf();

        // buf (userpayload)
        shared_ptr<SocketBuffer> curr_skbuf;
        for (curr_skbuf = skbuf_head; curr_skbuf;
             curr_skbuf = curr_skbuf->next) {
            int len =
                curr_skbuf->user_payload_end - curr_skbuf->user_payload_begin;
            buf->append(curr_skbuf->user_payload_begin, len);
        }

        if (require_addr) {
            // UDP only
            callRet->set_addr(
                std::string{ (char*)&peeraddr, sizeof(sockaddr_in) });
        }

        callRet->set_ret(buf->length());
        return;
    }

e_again:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto callRet = resp->mutable_recvfromcall();
        callRet->set_ret(EAGAIN);
        return;
    }

wait_next:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_WAIT_NEXT);

        {
            MUQUINETD_LOG(info) << "Currently no packet avavilable, EventLoop "
                                   "will wait for it. This function will "
                                   "return";
        }

        const auto& socket = rrChannel->socket();
        // 使用 weak_ptr 以避免 Socket ReqRespChannel 互相持有 shared_ptr
        //
        // 想要达到的设计目标是：Mux 持有 shared_ptr<ReqRespChannel>），
        //   对 ReqRespChannel 生命周期进行唯一管理；
        //   ReqRespChannel 持有 shared_ptr<Socket>，
        //   对 Socket 声明周期进行唯一管理。
        //
        // 由于下面语句中的 std::bind 会复制其参数，
        // 若我们将 shared_ptr 传给 std::bind，Socket 就会持有一份
        //   shared_ptr<ReqRespChannel>，形成 Socket 与 ReqRespChannel
        //   相互持有 shared_ptr 的局面
        // 这样会导致 ReqRespChannel 与 Socket 谁都不会被释放 --> 内存泄漏
        weak_ptr<ReqRespChannel> ch = rrChannel;
        socket->onAsyncNewPacket(std::bind(&RequestHandler::onAsyncNewUdpPacket,
                                           this, ch, require_addr));
        socket->setWaiting(true);

        return;
    }
}

void
RequestHandler::pollCall(const std::shared_ptr<ReqRespChannel>& rrChannel,
                         const std::shared_ptr<const Request>&,
                         const std::shared_ptr<Response>&)
{
}

void
RequestHandler::selectCall(const std::shared_ptr<ReqRespChannel>& rrChannel,
                           const std::shared_ptr<const Request>&,
                           const std::shared_ptr<Response>&)
{
}

void
RequestHandler::getpeernameCall(
    const std::shared_ptr<ReqRespChannel>& rrChannel,
    const std::shared_ptr<const Request>& req,
    const std::shared_ptr<Response>& resp)
{
    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    sockaddr_in sockname;

    {
        memset(&sockname, 0, sizeof(sockname));
        sockname.sin_family = AF_INET;
        sockname.sin_addr = pcb->faddr;
        sockname.sin_port = pcb->fport;
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_getpeernamecall();
        callRet->set_ret(0);
        // callRet->set_sockanme(&sockname, sizeof(struct sockaddr_in)); // <--
        // use
        // after free
        callRet->set_peername(
            std::string{ (char*)&sockname, sizeof(struct sockaddr_in) });
    }
}

void
RequestHandler::getsocknameCall(
    const std::shared_ptr<ReqRespChannel>& rrChannel,
    const std::shared_ptr<const Request>& req,
    const std::shared_ptr<Response>& resp)
{
    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    sockaddr_in sockname;

    {
        memset(&sockname, 0, sizeof(sockname));
        sockname.sin_family = AF_INET;
        sockname.sin_addr = pcb->laddr;
        sockname.sin_port = pcb->lport;
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_getsocknamecall();
        callRet->set_ret(0);
        // callRet->set_sockanme(&sockname, sizeof(struct sockaddr_in)); // <--
        // use
        // after free
        callRet->set_sockanme(
            std::string{ (char*)&sockname, sizeof(struct sockaddr_in) });
    }
}

void
RequestHandler::getsockoptCall(const std::shared_ptr<ReqRespChannel>& rrChannel,
                               const std::shared_ptr<const Request>&,
                               const std::shared_ptr<Response>&)
{
    // TODO
    // SOL_SOCKET .SO_ERROR
}

void
RequestHandler::setsockoptCall(const std::shared_ptr<ReqRespChannel>& rrChannel,
                               const std::shared_ptr<const Request>&,
                               const std::shared_ptr<Response>&)
{
    // Seems that we don't have to implement this API
}

void
RequestHandler::fcntlCall(const std::shared_ptr<ReqRespChannel>& rrChannel,
                          const std::shared_ptr<const Request>&,
                          const std::shared_ptr<Response>&)
{
    // TODO
    // F_GETFD / F_SETFD .FD_CLOEXEC
    // F_GETFL / F_SETFL .O_NONBLOCK
}

void
RequestHandler::atstartAction(const shared_ptr<ReqRespChannel>& rrChannel,
                              const shared_ptr<const Request>& req,
                              const shared_ptr<Response>& resp)
{
    const auto& call = req->atstartaction();
    const std::string& progname = call.progname();
    pid_t pid = req->pid();

    /*  1. 保存 peer 信息 */

    rrChannel->setPeerName(progname);
    rrChannel->addPeerPid(pid);

    /*  2. 返回信息给 Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto atstart = resp->mutable_atstartaction();
    atstart->set_startfd(4096);
    atstart->set_count(1024);
}

void
RequestHandler::atforkAction(const shared_ptr<ReqRespChannel>& rrChannel,
                             const shared_ptr<const Request>& req,
                             const shared_ptr<Response>& resp)
{
    rrChannel->increaseRefCount();
    // TODO what if CLOSE_EXEC
}

void
RequestHandler::atexitAction(const shared_ptr<ReqRespChannel>& rrChannel,
                             const shared_ptr<const Request>& req,
                             const shared_ptr<Response>& resp)
{
    // TODO
    // 找到本进程所有的 ReqRespChannel，对其 refcnt--
    // 若 refcnt 减到 0，销毁
}

void
RequestHandler::onAsyncNewUdpPacket(
    const weak_ptr<ReqRespChannel>& rrChannel_weak, bool require_addr)
{
    shared_ptr<ReqRespChannel> rrChannel = rrChannel_weak.lock();
    if (!rrChannel)
        return;

    const auto& so = rrChannel->socket();

    shared_ptr<SocketBuffer> skbuf_head;
    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(sockaddr_in));

    // take packet
    so->takeFromRecvQ(peeraddr, skbuf_head);

    // prepare Response
    auto resp = make_shared<Response>();
    auto* callRet = resp->mutable_recvfromcall();
    std::string* buf = callRet->mutable_buf();

    // buf (userpayload)
    shared_ptr<SocketBuffer> curr_skbuf;
    for (curr_skbuf = skbuf_head; curr_skbuf; curr_skbuf = curr_skbuf->next) {
        int len = curr_skbuf->user_payload_end - curr_skbuf->user_payload_begin;
        buf->append(curr_skbuf->user_payload_begin, len);
    }

    // addr
    if (require_addr) {
        callRet->set_addr(std::string{ (char*)&peeraddr, sizeof(sockaddr_in) });
    }

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);
    callRet->set_ret(buf->length());
    rrChannel->write(resp);
}

void
RequestHandler::onAsyncNewTcpPacket(const weak_ptr<ReqRespChannel>&)
{
    // TODO
}
