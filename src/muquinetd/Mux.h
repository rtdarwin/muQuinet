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

#ifndef MUQUINETD_MUX_H
#define MUQUINETD_MUX_H

#include <memory>

#include "muquinetd/base/Singleton.h"

class SocketBuffer;
class EventLoop;
class ReqRespChannel;

class Mux : public Singleton<Mux>
{
    friend class Singleton<Mux>;

public:
    void init();
    void start();
    void stop();

    void addRRChannel(const std::shared_ptr<ReqRespChannel>&);
    void removeRRChannel(const std::shared_ptr<ReqRespChannel>&);

    EventLoop* eventLoop();

private:
    Mux();
    ~Mux();

    struct Impl; // pimpl forward declaration
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_MUX_H
