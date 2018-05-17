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

#ifndef MUQUINETD_UDP_H
#define MUQUINETD_UDP_H

#include "muquinetd/base/Singleton.h"
#include <netinet/in.h>
#include <memory>

class SocketBuffer;
class Pcb;

class Udp : public Singleton<Udp>
{
    friend class Singleton<Udp>;

public:
    void init();
    void start();
    void stop();

    std::shared_ptr<Pcb> newPcb();
    void removePcb(const std::shared_ptr<const Pcb>&);

    // For IP Layer use
    void rx(const std::shared_ptr<SocketBuffer>&);

private:
    Udp();
    ~Udp();

    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_UDP_H
