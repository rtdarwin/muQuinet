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

#include "muquinetd/ip/Defrager.h"

#include <algorithm>
#include <assert.h>
#include <list>

#include "muquinetd/Logging.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/base/MutexLock.h"
#include "muquinetd/ip/IpHeader.h"
#include "muquinetd/ip/frag/DefragContext.h"

using std::shared_ptr;
using std::unique_ptr;

namespace {

// for timeout_cb use
std::list<DefragContext>* ctxList = nullptr;
MutexLock* lock;

void
timeout_cb(EV_P_ ev_timer* w, int /* unused */ revents)
{
    assert(ctxList != nullptr);

    // C-style programming

    // stop timer
    ev_timer_stop(EV_A_ w);

    MutexLockGuard l(*lock);

    // 定位并从列表中删除
    //  由于 timeout 与 分片到齐 的情况可能并发的发生，到这个地方的时候，
    //  ctx 可能刚好从列表中移除，所以这时我们不能假定 ctx 一定还存在于列表中
    DefragContext* ctx = (DefragContext*)(w - offsetof(DefragContext, timer));
    ctxList->remove_if([ctx](const DefragContext& cToPred) {
        if (&cToPred == ctx) {
            return true;
        } else {
            return false;
        }
    });
}
} // namespace {

struct Defrager::Impl
{
    std::list<DefragContext> defragCtxList;
    MutexLock lock;
};

Defrager::Defrager()
{
    _pImpl.reset(new Defrager::Impl);
    ctxList = &_pImpl->defragCtxList;
    lock = &_pImpl->lock;
}

Defrager::~Defrager() = default;

shared_ptr<SocketBuffer>
Defrager::defrag(const std::shared_ptr<SocketBuffer>& skbuf)
{
    MUQUINETD_LOG(info) << "defragging";

    shared_ptr<SocketBuffer> packet;
    DefragContext* ctx = nullptr;
    IpHeader* iphdr = (IpHeader*)skbuf->network_hdr;

    MutexLockGuard l(_pImpl->lock);

    /*  1. 查找是否已有同分组的分片 */

    for (DefragContext& c : _pImpl->defragCtxList) {
        if (c.ip_id == iphdr->id && c.ip_protocol == iphdr->protocol &&
            c.ip_saddr == iphdr->saddr && c.ip_daddr == iphdr->daddr) {
            ctx = &c;
            break;
        }
    }

    /*  2. 没 就创建一个新的分组 ctx */

    if (!ctx) {
        MUQUINETD_LOG(debug) << "Add one DefragContext to DefragContextList";
        _pImpl->defragCtxList.emplace_back();
        ctx = &_pImpl->defragCtxList.back();

        ctx->ip_id = iphdr->id;
        ctx->ip_protocol = iphdr->protocol;
        ctx->ip_saddr = iphdr->saddr;
        ctx->ip_daddr = iphdr->daddr;

        struct ev_loop* loop = EV_DEFAULT;
        ev_timer_init(&ctx->timer, &timeout_cb, 60,
                      0); // 60 seconds as RFC 1122
        ev_timer_start(EV_A_ & ctx->timer);
    }

    /*  3. 插入到分组中 */

    // clang-format off
    if (!ctx->first
        || ( __be16_to_cpu(((IpHeader*)ctx->first->network_hdr)->frag_off)
             & IP_OFF_MASK )
           > ( __be16_to_cpu(iphdr->frag_off) & IP_OFF_MASK))
    {
        skbuf->next = ctx->first;
        ctx->first = skbuf;
    }
    else
    {
        shared_ptr<SocketBuffer> prev;
        shared_ptr<SocketBuffer> curr = ctx->first;

        int ours_off = iphdr->frag_off & IP_OFF_MASK;
        while (curr &&
               ours_off > (__be16_to_cpu(((IpHeader*)curr->network_hdr)->frag_off)
                           & IP_OFF_MASK)) {
            prev = curr;
            curr = curr->next;
        }

        prev->next = skbuf;
        skbuf->next = curr;
    }
    // clang-format on

    /*  4. 分组的分片齐了吗？
     */

    shared_ptr<SocketBuffer> curr = ctx->first;
    iphdr = nullptr;
    __le16 frag_off;
    int expectedOff = 0;
    __le16 payloadlen = 0;

    // 判断 第一个分片到最后一个分片 每个分片中的片偏移是否连续
    for (curr = ctx->first; curr; curr = curr->next) {
        iphdr = (IpHeader*)curr->network_hdr;
        int currOff = (__be16_to_cpu((iphdr->frag_off)) & IP_OFF_MASK) * 8;
        if (currOff != expectedOff)
            goto packet_not_ready;

        expectedOff = currOff + __be16_to_cpu(iphdr->tot_len) - iphdr->ihl * 4;
    }

    // 判断 最后一个分片 MF 是否为 0
    for (curr = ctx->first; curr->next; curr = curr->next)
        ;
    iphdr = (IpHeader*)curr->network_hdr;
    frag_off = __be16_to_cpu(iphdr->frag_off);
    if ((frag_off & IP_MF_MASK) != 0) {
        goto packet_not_ready;
    }
    payloadlen = (frag_off & IP_OFF_MASK) * 8 + __be16_to_cpu(iphdr->tot_len) -
                 iphdr->ihl * 4;

    {
        MUQUINETD_LOG(info) << "All IP fragments arrived, reassemblying";
    }

    /*  5. 重装分组 后 删除重装上下文 */

    // 调整第一个分片
    packet = ctx->first;
    iphdr = (IpHeader*)packet->network_hdr;
    iphdr->tot_len = __cpu_to_be16(payloadlen + iphdr->ihl * 4);

    // 调整后续的分片
    for (curr = ctx->first->next; curr; curr = curr->next) {
        iphdr = (IpHeader*)curr->network_hdr;
        int iphdrlen = iphdr->ihl;
        curr->network_hdr = nullptr;
        curr->user_payload_begin = curr->rawBytes + iphdrlen;
    }

    {
        MUQUINETD_LOG(info)
            << "Reassemblying finished, IP packet {payload length = "
            << payloadlen << "}";
    }

    ctxList->remove_if([ctx](const DefragContext& cToPred) {
        if (&cToPred == ctx) {
            MUQUINETD_LOG(debug)
                << "Remove one DefragContext from DefragContextList";
            return true;
        } else {
            return false;
        }
    });

packet_not_ready:

    MUQUINETD_LOG(debug) << "DefragContextList.size = "
                         << _pImpl->defragCtxList.size();

    return packet;
}
