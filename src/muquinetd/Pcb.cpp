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

#include "muquinetd/Pcb.h"

#include <arpa/inet.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "muquinetd/Conf.h"
#include "muquinetd/Logging.h"

using std::weak_ptr;
using std::shared_ptr;

Pcb::Pcb()
{
    {
        MUQUINETD_LOG(debug) << "New Pcb";
    }

    bzero(&faddr, sizeof(struct in_addr));
    bzero(&fport, sizeof(__be16));

    bzero(&laddr, sizeof(struct in_addr));
    {
        const string& addr_str = Conf::get()->stack.addr;
        inet_pton(AF_INET, addr_str.c_str(), &laddr);
    }

    bzero(&lport, sizeof(__be16));
}

Pcb::~Pcb()
{
    MUQUINETD_LOG(debug) << "Destroy Pcb";
}

int
Pcb::connect(const struct in_addr& faddr, __be16 fport)
{
    this->faddr = faddr;
    this->fport = fport;

    if (lport == 0) {
        this->bind();
    }

    return 0;
}

void
Pcb::setOnConnEstabCB(const std::function<void()>&)
{
}

SelectableChannel*
Pcb::getConnEstabNotifyChannel()
{
    return nullptr;
}

void
Pcb::disconnect()
{
    bzero(&faddr, sizeof(struct in_addr));
    fport = 0;
}

int
Pcb::send(const std::string& buf)
{
    if (lport == 0) {
        this->bind();
    }

    if (fport == 0) {
        return ENOTCONN;
    }
    return 0;
}

int
Pcb::send(const struct in_addr& faddr, __be16 fport, const std::string& buf)
{
    if (lport == 0) {
        this->bind();
    }

    if (fport != 0) {
        return EISCONN;
    }
    return 0;
}

void
Pcb::recv(const std::shared_ptr<SocketBuffer>&)
{
    // nothing need to do
}

void
Pcb::recv(sockaddr_in& peeraddr, const std::shared_ptr<SocketBuffer>&)
{
    // TODO
}

int
Pcb::bind()
{
    if (lport == 0) {
        lport = nextAvailLocalPort();
    }
}

weak_ptr<Socket>
Pcb::socket()
{
    return weak_ptr<Socket>{ _socket };
}

void
Pcb::setSocket(const std::weak_ptr<Socket>& so)
{
    _socket = so;
}
