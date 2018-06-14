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

#include "muquinetd/Mux.h"

#include <boost/thread/thread.hpp>
#include <memory>

#include "muquinetd/Logging.h"
#include "muquinetd/mux/EventLoop.h"
#include "muquinetd/mux/RequestHandler.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/mux/req-resp-channel/MasterListener.h"
#include "muquinetd/mux/req-resp-channel/ReqRespChannel.h"
// #include "rpc/cpp_out/request.pb.h"
// #include "rpc/cpp_out/response.pb.h"

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using boost::thread;
using std::vector;

class Request;
class Response;

struct Mux::Impl
{
    EventLoop eventloop;
    shared_ptr<MasterListener> masterListener;
    shared_ptr<RequestHandler> reqHandler;
    std::list<shared_ptr<ReqRespChannel>>
        rrChannels; // rrChanel -> Socket -> Pcb
};

// Reason that defaulted ctor/dtor definitions here:
//  https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile/32269374
Mux::Mux()
{
    _pImpl.reset(new Mux::Impl());
}

Mux::~Mux() = default;

void
Mux::init()
{
    /*  1. MasterListener : handle new connection from interceptors */

    shared_ptr<MasterListener> l = make_shared<MasterListener>();
    l->onNewConnection([this](int connfd) {
        MUQUINETD_LOG(info) << "New conncection from interceptor";

        // New interceptor connection
        // -> New ReqRespChannel (rrChannel)
        auto rrChannel = make_shared<ReqRespChannel>(connfd);
        this->addRRChannel(rrChannel);

        // clang-format off
        rrChannel->setOnNewRequestCB
        (
            [this]
            (const std::shared_ptr<ReqRespChannel>& rrChannel
             , const std::shared_ptr<const Request>& req)
            -> shared_ptr<Response>
            {
              shared_ptr<ReqRespChannel> holdit = rrChannel;
                MUQUINETD_LOG(info) << "New Request from interceptor";
                return this->_pImpl->reqHandler->handleRequest(rrChannel, req);
            }
        );
        // clang-format on

        auto sChannel = rrChannel->getSelectableChannel();
        this->_pImpl->eventloop.addChannel(sChannel);
        sChannel->setOwnerEventLoop(&this->_pImpl->eventloop);
    });

    _pImpl->masterListener = l;
    SelectableChannel* listenerCh = l->getSelectableChannel().get();
    EventLoop& eventloop = _pImpl->eventloop;

    // Let eventloop and channel know each other
    eventloop.addChannel(listenerCh);
    listenerCh->setOwnerEventLoop(&eventloop);

    /*  2. RequestHandler : handle request from interceptors */

    auto handler = make_shared<RequestHandler>();
    _pImpl->reqHandler = handler;
}

void
Mux::start()
{
    MUQUINETD_LOG(debug) << "Starting event loop thread";
    thread eventLoopThread([this]() {
        LoggingThreadInitializer i;
        i.run();

        MUQUINETD_LOG(debug) << "Event loop thread begins to work";

        this->_pImpl->masterListener->listen();
        this->_pImpl->eventloop.loop();
    });
    MUQUINETD_LOG(debug) << "Started event loop thread";
}

void
Mux::stop()
{
    _pImpl->eventloop.stop();
}

void
Mux::addRRChannel(const std::shared_ptr<ReqRespChannel>& ch)
{
    _pImpl->rrChannels.push_back(ch);
    MUQUINETD_LOG(debug) << "Adding a RRChannel";
}

void
Mux::removeRRChannel(const std::shared_ptr<ReqRespChannel>& ch)
{
    _pImpl->rrChannels.remove_if(
        [&ch](const std::shared_ptr<ReqRespChannel>& chToPred) -> bool {
            if (chToPred.get() == ch.get()) {
                MUQUINETD_LOG(debug) << "Removing a RRChannel";
                return true;
            }
            return false;
        });
}

EventLoop*
Mux::eventLoop()
{
    return &this->_pImpl->eventloop;
}
