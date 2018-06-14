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

#ifndef MUQUINETD_MUX_REQRESPCHANNEL_REQUESTHANDLER_H
#define MUQUINETD_MUX_REQRESPCHANNEL_REQUESTHANDLER_H

#include <memory>

class Request;
class Response;
class ReqRespChannel;

class RequestHandler
{
public:
    RequestHandler() = default;
    ~RequestHandler() = default;
    // Non-copyable, Non-moveable
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    RequestHandler(RequestHandler&&) = delete;
    RequestHandler& operator=(RequestHandler&&) = delete;

    std::shared_ptr<Response> handleRequest(
        const std::shared_ptr<ReqRespChannel>& rrChannel,
        const std::shared_ptr<const Request>& req);

private:
    // clang-format off

    /* 1. create/connect/close */

    void socketCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                    , const std::shared_ptr<const Request>&
                    , const std::shared_ptr<Response>&);
    void connectCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                     , const std::shared_ptr<const Request>&
                     , const std::shared_ptr<Response>&);
    void closeCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                   , const std::shared_ptr<const Request>&
                   , const std::shared_ptr<Response>&);

    /* 2. read/write */

    void sendtoCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                    , const std::shared_ptr<const Request>&
                    , const std::shared_ptr<Response>&);
    void recvfromCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                      , const std::shared_ptr<const Request>&
                      , const std::shared_ptr<Response>&);

    /* 3. poll */

    void pollCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                  , const std::shared_ptr<const Request>&
                  , const std::shared_ptr<Response>&);
    void selectCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                    , const std::shared_ptr<const Request>&
                    , const std::shared_ptr<Response>&);

    /* 4. options */

    void getpeernameCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                         , const std::shared_ptr<const Request>&
                         , const std::shared_ptr<Response>&);
    void getsocknameCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                         , const std::shared_ptr<const Request>&
                         , const std::shared_ptr<Response>&);
    void getsockoptCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                        , const std::shared_ptr<const Request>&
                        , const std::shared_ptr<Response>&);
    void setsockoptCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                        , const std::shared_ptr<const Request>&
                        , const std::shared_ptr<Response>&);
    void fcntlCall(const std::shared_ptr<ReqRespChannel>& rrChannel
                   , const std::shared_ptr<const Request>&
                   , const std::shared_ptr<Response>&);

    /* 5. process information */

    void atstartAction(const std::shared_ptr<ReqRespChannel>& rrChannel
                       , const std::shared_ptr<const Request>&
                       , const std::shared_ptr<Response>&);
    // clang-format on

    // weak_ptr rather than shared_ptr is essential
    void onAsyncNewUdpPacket(const std::weak_ptr<ReqRespChannel>&,
                             bool require_addr);
    void onAsyncNewTcpPacket(const std::weak_ptr<ReqRespChannel>&);
};
#endif
