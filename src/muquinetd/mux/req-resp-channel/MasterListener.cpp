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

#include "MasterListener.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "muquinetd/Logging.h"
#include "muquinetd/muQuinetd.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::make_shared;

struct MasterListener::Impl
{
    shared_ptr<SelectableChannel> listenChannel;
    int fd;

    std::function<void(int connfd)> onNewConnectionFunc;

    void onReadReady();
};

MasterListener::MasterListener()
{
    _pImpl.reset(new MasterListener::Impl);

    /*  1. 准备 创建/绑定 UNIX socket 所需的数据 */

    const char* socketPath =
        RPC_MASTER_LISTENER_SOCKET_DIR "/" RPC_MASTER_LISTENER_SOCKET_NAME;

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, socketPath, sizeof(sa.sun_path) - 1);

    /*  2. 创建/绑定 UNIX socket */

    // 保证父目录存在 & 删除可能存在的遗留文件
    ::mkdir(RPC_MASTER_LISTENER_SOCKET_DIR,
            S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    unlink(socketPath);

    int listenfd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    int ret =
        bind(listenfd, (const struct sockaddr*)&sa, sizeof(struct sockaddr_un));
    if (ret == -1) {
        int errno_ = errno;
        MUQUINETD_LOG(fatal) << "Failed to create MasterListener socket: "
                             << std::string(strerror(errno_));
        muQuinetd::get()->stop();
        muQuinetd::get()->exit(muQuinetd::exit_status::FAILURE);
    }

    MUQUINETD_LOG(debug) << "MasterListener fd: " << listenfd;

    /*  3. SelectableChannel (for EventLoop use) of self */

    _pImpl->fd = listenfd;

    auto listenCh = make_shared<SelectableChannel>(listenfd);
    listenCh->onRead([this]() { this->_pImpl->onReadReady(); });
    listenCh->enableReading();
    _pImpl->listenChannel = listenCh;
}

MasterListener::~MasterListener()
{
    ::close(_pImpl->fd);
    ::unlink(RPC_MASTER_LISTENER_SOCKET_DIR
             "/" RPC_MASTER_LISTENER_SOCKET_NAME);
}

void
MasterListener::listen()
{
    int ret = ::listen(_pImpl->fd, 20);
    if (ret == -1) {
        int errno_ = errno;
        MUQUINETD_LOG(fatal) << "Failed to listen MasterListener socket: "
                             << std::string(strerror(errno_));
        muQuinetd::get()->stop();
        muQuinetd::get()->exit(muQuinetd::exit_status::FAILURE);
    }
}

std::shared_ptr<SelectableChannel>
MasterListener::getSelectableChannel()
{
    return _pImpl->listenChannel;
}

void
MasterListener::onNewConnection(const std::function<void(int sockfd)>& callback)
{
    _pImpl->onNewConnectionFunc = callback;
}

void
MasterListener::Impl::onReadReady()
{
    auto listenfd = this->fd;
    auto callback = this->onNewConnectionFunc;

    /*  1. accept */

    int connfd;
    while ((connfd = ::accept(listenfd, NULL, NULL)) != -1) {
        if (callback) {
            callback(connfd);
        }
    }

    /*  2. check errno */

    int errno_ = errno;
    if (errno_ != EWOULDBLOCK) {
        MUQUINETD_LOG(error)
            << "MasterListener.accept: " << std::string(strerror(errno_));
    }
}
