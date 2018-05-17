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

#ifndef MUQUINETD_IP_H
#define MUQUINETD_IP_H

#include <memory>

#include "muquinetd/base/Singleton.h"

class SocketBuffer;

class Ip : public Singleton<Ip>
{
    friend class Singleton<Ip>;

public:
    void init();
    void start();
    void stop();

    // For Transport layer use
    void tx(const std::shared_ptr<SocketBuffer>& skbuf,
            const std::string& user_payload);

    // For Interface layer use
    void enRxQue(const std::shared_ptr<SocketBuffer>& skbuf);

private:
    Ip();
    ~Ip();

    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_IP_H
