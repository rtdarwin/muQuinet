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

#ifndef MUQUINETD_PCB_H
#define MUQUINETD_PCB_H

#include <asm/byteorder.h>
#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <memory>

class Socket;
class SocketBuffer;

class Pcb : public std::enable_shared_from_this<Pcb>
{
public:
    struct in_addr faddr;
    __be16 fport;
    struct in_addr laddr;
    __be16 lport;

public:
    Pcb();
    virtual ~Pcb();
    // Non-copyable, Non-moveable
    Pcb(const Pcb&) = delete;
    Pcb& operator=(const Pcb&) = delete;
    Pcb(Pcb&&) = delete;
    Pcb& operator=(Pcb&&) = delete;

    int bind();
    virtual int connect(const struct in_addr& faddr, __be16 fport);
    void disconnect();
    virtual int send(const std::string& buf);
    virtual int send(const struct in_addr& faddr, __be16 fport,
                     const std::string& buf);
    virtual void recv(const std::shared_ptr<SocketBuffer>&);
    virtual void recv(struct sockaddr_in& peeraddr, const std::shared_ptr<SocketBuffer>&);
    virtual __be16 nextAvailLocalPort() = 0; // each protocol implements
    // void bind(struct in_addr laddr, __be16 lport);

    std::weak_ptr<Socket> socket();
    void setSocket(const std::shared_ptr<Socket>&);

private:
    std::shared_ptr<Socket> _socket;
};

class Pcbs
{
public:
    template <typename Container>
    static std::shared_ptr<Pcb> find(Container c, struct in_addr faddr,
                                     __be16 fport, struct in_addr laddr,
                                     __be16 lport)
    {
        // clang-format off
        std::function< int
                      (const struct in_addr& ours, const struct in_addr& theirs)
                     > in_addr_match
             = [](const struct in_addr& ours, const struct in_addr& theirs)
               -> int
               {
                   uint32_t ours_ = *(uint32_t*)&ours;
                   uint32_t theirs_ = *(uint32_t*)&theirs;

                   if (ours_ == 0){
                     return 0;
                   } else if (ours_ == theirs_){
                     return 1;
                   } else {
                     return -1;
                   }
               };
        // clang-format on

        typename Container::const_iterator it;
        std::shared_ptr<Pcb> pcb;

        int match = 0;
        for (it = c.cbegin(); it != c.cend(); ++it) {
            const std::shared_ptr<Pcb>& curr = *it;
            int accum = 0;
            int m = 0;

            {
                if (curr->lport != lport) {
                    continue;
                } else {
                    accum += 1;
                }

                if (curr->fport != 0) {
                    if (curr->fport == fport) {
                        accum += 1;
                    } else {
                        continue;
                    }
                } else {
                    //  accum += 0
                }

                if ((m = in_addr_match(curr->faddr, faddr)) == -1) {
                    continue;
                }
                accum += m; // 0 or 1

                if ((m = in_addr_match(curr->laddr, laddr)) == -1) {
                    continue;
                }
                accum += m; // 0 or 1
            }

            if (accum > m) {
                pcb = curr;
                m = accum;
            }
        }

        return pcb;
    }
};

#endif
