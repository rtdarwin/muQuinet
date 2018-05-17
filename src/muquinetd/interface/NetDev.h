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

#ifndef MUQUINETD_INTERFACE_NETDEV_H
#define MUQUINETD_INTERFACE_NETDEV_H

#include <functional>
#include <memory>

class SocketBuffer;

class NetDev
{
public:
    NetDev() = default;
    virtual ~NetDev();
    // Well, non-copyable & non-moveable
    NetDev(const NetDev&) = delete;
    NetDev(NetDev&&) = delete;
    NetDev& operator=(const NetDev*) = delete;
    NetDev& operator=(NetDev*) = delete;

    virtual void init();
    virtual void rx(std::shared_ptr<SocketBuffer> skbuf);
    virtual void tx(const std::shared_ptr<SocketBuffer>& skbuf);
    virtual void close();
};

#endif // MUQUINETD_INTERFACE_NETDEV_H
