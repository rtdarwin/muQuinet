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

#include "muquinetd/Tcp.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <list>

#include "muquinetd/Logging.h"
#include "muquinetd/Pcb.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/tcp/TcpPcb.h"

using std::unique_ptr;
using std::make_shared;
using std::shared_ptr;

struct Tcp::Impl
{
    std::list<shared_ptr<Pcb>> pcbs;
};

Tcp::Tcp()
{
    _pImpl.reset(new Tcp::Impl);
}

Tcp::~Tcp() = default;

void
Tcp::init()
{
    // Nothing need to do
}

void
Tcp::start()
{
    // Nothing need to do
}

void
Tcp::stop()
{
}

shared_ptr<Pcb>
Tcp::newPcb()
{
    const auto& pcb = make_shared<TcpPcb>();
    _pImpl->pcbs.push_back(pcb);

    return pcb;
}

void
Tcp::removePcb(const std::shared_ptr<const Pcb>& pcb)
{
    std::remove(_pImpl->pcbs.begin(), _pImpl->pcbs.end(), pcb);
}

void
Tcp::rx(const shared_ptr<SocketBuffer>& skbuf)
{
    // TODO
}

void
Tcp::tx()
{
}
