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

#ifndef MUQUINETD_REQRESPCHANNEL_MASTERLISTENER_H
#define MUQUINETD_REQRESPCHANNEL_MASTERLISTENER_H

#include <functional>
#include <memory>

class SelectableChannel;

class MasterListener
{
public:
    MasterListener();
    ~MasterListener();
    // Non-copyable, Non-moveable
    MasterListener(const MasterListener&) = delete;
    MasterListener& operator=(const MasterListener&) = delete;
    MasterListener(MasterListener&&) = delete;
    MasterListener& operator=(MasterListener&&) = delete;

    void listen();
    // clang-format off
    void onNewConnection(
        const std::function
        <
         void(int sockfd)
        > &
    );
    // clang-format on

    // MasterListener 对 SelectableChannel 生命周期负责
    std::shared_ptr<SelectableChannel> getSelectableChannel();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_REQRESPCHANNEL_MASTERLISTENER_H
