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

#ifndef MUQUINETD_INTERFACE_TUNDEVICE_H
#define MUQUINETD_INTERFACE_TUNDEVICE_H

#include <memory>

#include "NetDev.h"

class SocketBuffer;

class TunDevice : public NetDev
{
public:
    TunDevice() = default;
    virtual ~TunDevice() override;

    virtual void init() override;
    virtual void rx(std::shared_ptr<SocketBuffer> skbuf) override;
    virtual void tx(const std::shared_ptr<SocketBuffer>& skbuf) override;
    virtual void close() override;

private:
    void allocateTun();
    void setIfUp();
    void setIfAddr();

    char* _devname = nullptr;
    int _fd = 0;
};

#endif // MUQUINETD_INTERFACE_TUNDEVICE_H
