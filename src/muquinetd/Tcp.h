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

#ifndef MUQUINETD_TCP_H
#define MUQUINETD_TCP_H

#include "muquinetd/base/Singleton.h"
#include <memory>

class SocketBuffer;
class Pcb;

class Tcp : public Singleton<Tcp>
{
    friend class Singleton<Tcp>;

public:
    void init();
    void start();
    void stop();

    std::shared_ptr<Pcb> newPcb();
    void removePcb(const std::shared_ptr<const Pcb>&);

    void rx(const std::shared_ptr<SocketBuffer>&);
    void tx();

private:
    Tcp();
    ~Tcp();

    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_TCP_H
