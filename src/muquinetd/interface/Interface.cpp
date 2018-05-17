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

#include "muquinetd/Interface.h"

#include <boost/thread/thread.hpp>
#include <memory>

#include "muquinetd/Ip.h"
#include "muquinetd/Logging.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/interface/NetDev.h"
#include "muquinetd/interface/TunDevice.h"

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using boost::thread;

struct Interface::Impl
{
    bool stopflag = false;

    unique_ptr<NetDev> tunDev;
};

// Reason that defaulted ctor/dtor definitions here:
//  https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile/32269374
Interface::Interface()
{
    _pImpl.reset(new Interface::Impl);
}

Interface::~Interface() = default;

void
Interface::init()
{
    auto tun = new TunDevice();
    tun->init();
    _pImpl->tunDev.reset(tun);
}

void
Interface::start()
{
    MUQUINETD_LOG(debug) << "Starting TUN device rx_loop_thread";
    thread rxLoop([this]() {
        LoggingThreadInitializer i;
        i.run();

        MUQUINETD_LOG(info) << "TUN device rx_loop_thread begins to work";
        while (!this->_pImpl->stopflag) {
            shared_ptr<SocketBuffer> skbuf = make_shared<SocketBuffer>();
            this->_pImpl->tunDev->rx(skbuf);
            MUQUINETD_LOG(info) << "Interface Layer received a packet from TUN "
                                   "device, passing it to IP Layer";
            Ip::get()->enRxQue(skbuf);
        }
    });
    MUQUINETD_LOG(debug) << "Started TUN device rx_loop_thread";
}

void
Interface::stop()
{
    MUQUINETD_LOG(info) << "Stopping Interface Layer";
    _pImpl->stopflag = true;
}

void
Interface::tx(const shared_ptr<SocketBuffer>& skbuf)
{
    _pImpl->tunDev->tx(skbuf);
}
