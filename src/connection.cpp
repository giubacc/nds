/* Original Work Copyright (c) 2021 Giuseppe Baccini - giuseppe.baccini@live.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef __GNUG__
#include <errno.h>
#include <unistd.h>
#endif
#include "peer.h"

#define RCV_SND_BUF_SZ 8192
#define WORD_SZ 4   //word length [byte size]

namespace nds {

//connection

connection::connection(selector &sel, ConnectionType ct) :
    sel_(sel),
    con_type_(ct),
    status_(ConnectionStatus_DISCONNECTED),
    socket_(INVALID_SOCKET),
    pkt_ch_st_(PktChasingStatus_BodyLen),
    bdy_bytelen_(0),
    rdn_buff_(RCV_SND_BUF_SZ),
    acc_snd_buff_(RCV_SND_BUF_SZ),
    log_(sel.log_)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
}

std::shared_ptr<connection> &connection::self_as_shared_ptr()
{
    if(sel_.mcast_udp_inco_conn_->socket_ == socket_) {
        return sel_.mcast_udp_inco_conn_;
    }

    auto it = sel_.inco_conn_map_.find(socket_);
    if(it != sel_.inco_conn_map_.end()) {
        return it->second;
    }
    it = sel_.outg_conn_map_.find(socket_);
    if(it != sel_.outg_conn_map_.end()) {
        return it->second;
    }
    log_->critical("self_as_shared_ptr: no such connection, exit");
    throw;
}

const char *connection::get_host_ip() const
{
    if(socket_ == INVALID_SOCKET) {
        return "invalid address";
    }
    sockaddr_in saddr;
    socklen_t len = sizeof(saddr);
    getpeername(socket_, (sockaddr *)&saddr, &len);
    return inet_ntoa(saddr.sin_addr);
}

unsigned short connection::get_host_port() const
{
    if(socket_ == INVALID_SOCKET) {
        return 0;
    }
    sockaddr_in saddr;
    socklen_t len = sizeof(saddr);
    getpeername(socket_, (sockaddr *)&saddr, &len);
    return ntohs(saddr.sin_port);
}

void connection::set_host_ip(const char *ip)
{
    addr_.sin_addr.s_addr = inet_addr(ip);
}

void connection::set_host_port(unsigned short port)
{
    addr_.sin_port = htons(port);
}

RetCode connection::set_socket_blocking_mode(bool blocking)
{
    int flags = fcntl(socket_, F_GETFL, 0);
    if(flags < 0) {
        return RetCode_KO;
    }
    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    return (fcntl(socket_, F_SETFL, flags) == 0) ? RetCode_OK : RetCode_KO;
}

RetCode connection::sckt_hndl_err(long sock_op_res)
{
    RetCode rcode = RetCode_OK;
    if(sock_op_res == SOCKET_ERROR) {
        int err = errno;
        if(err == EAGAIN || err == EWOULDBLOCK) {
            rcode = RetCode_SCKWBLK;
        } else if(err == ECONNRESET) {
            rcode = RetCode_SCKCLO;
        } else {
            rcode = RetCode_SCKERR;
        }
    } else if(!sock_op_res) {
        rcode = RetCode_SCKCLO;
    } else {
        rcode = RetCode_UNKERR;
    }

    switch((rcode)) {
        case RetCode_SCKCLO:
            close_connection();
            break;
        case RetCode_SCKERR:
        case RetCode_UNKERR:
            close_connection();
            break;
        default:
            break;
    }
    return rcode;
}

RetCode connection::establish_multicast(sockaddr_in &params)
{
    if(con_type_ != ConnectionType_UDP_INGOING &&
            con_type_ != ConnectionType_UDP_OUTGOING) {
        return RetCode_KO;
    }

    log_->debug("establish_multicast [{}] -> mcast:{} - port:{}",
                (con_type_ == ConnectionType_UDP_INGOING) ? "receiver" : "sender",
                inet_ntoa(params.sin_addr),
                htons(params.sin_port));

    addr_ = params;
    RetCode rcode = RetCode_OK;

    if((socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != INVALID_SOCKET) {

        if(con_type_ == ConnectionType_UDP_OUTGOING) {
            //sender

            unsigned char ttl = 2;
            if(setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) < 0) {
                log_->critical("setsockopt IP_MULTICAST_TTL:{}", errno);
                return RetCode_KO;
            }
        } else {
            //receiver

            u_int yes = 1;
            if(setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(yes)) < 0) {
                log_->critical("setsockopt SO_REUSEADDR:{}", errno);
                return RetCode_KO;
            }

            struct sockaddr_in maddr;
            memset(&maddr, 0, sizeof(maddr));
            maddr.sin_family = AF_INET;
            maddr.sin_addr.s_addr = htonl(INADDR_ANY);
            maddr.sin_port = addr_.sin_port;

            if(bind(socket_, (struct sockaddr *)&maddr, sizeof(struct sockaddr_in)) < 0) {
                log_->critical("bind errno:{}", errno);
                return RetCode_KO;
            }

            struct ip_mreq mreq;
            memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = addr_.sin_addr.s_addr;
            mreq.imr_interface.s_addr = INADDR_ANY;

            if(setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
                log_->critical("setsockopt IP_ADD_MEMBERSHIP:{}", errno);
                return RetCode_KO;
            }
        }

    } else {
        log_->critical("socket:{}", errno);
        return RetCode_KO;
    }

    set_connection_established();
    return rcode;
}

RetCode connection::establish_connection()
{
    return establish_connection(addr_);
}

RetCode connection::establish_connection(sockaddr_in &params)
{
    if(con_type_ == ConnectionType_UDP_INGOING ||
            con_type_ == ConnectionType_UDP_OUTGOING) {
        return RetCode_KO;
    }

    log_->debug("establish_connection -> host:{} - port:{}",
                inet_ntoa(params.sin_addr),
                htons(params.sin_port));

    addr_ = params;
    RetCode rcode = RetCode_OK;
    int connect_res = 0;
    if((socket_ = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
        socklen_t len = sizeof(sockaddr_in);
        if((connect_res = connect(socket_, (struct sockaddr *)&addr_, len)) != INVALID_SOCKET) {
            log_->debug("connect OK -> host:{} - port:{}",
                        inet_ntoa(addr_.sin_addr),
                        htons(addr_.sin_port));
        } else {
            log_->error("connect KO -> host:{} - port:{} - errno:{}",
                        inet_ntoa(addr_.sin_addr),
                        htons(addr_.sin_port),
                        errno);
        }
    } else {
        log_->critical("errno:{}", errno);
    }

    if(!connect_res) {
        rcode = set_connection_established();
    } else {
        close_connection();
        rcode = RetCode_KO;
    }
    return rcode;
}

RetCode connection::set_connection_established()
{
    if(set_socket_blocking_mode(false)) {
        log_->critical("set socket not blocking");
        return RetCode_KO;
    }

    if(con_type_ != ConnectionType_UDP_INGOING &&
            con_type_ != ConnectionType_UDP_OUTGOING) {
        log_->debug("connection established: socket:{}, host:{}, port:{}",
                    socket_,
                    inet_ntoa(addr_.sin_addr),
                    ntohs(addr_.sin_port));
    }

    status_ = ConnectionStatus_ESTABLISHED;
    on_established();
    return RetCode_OK;
}

RetCode connection::close_connection()
{
    socket_shutdown();
    reset_rdn_outg_rep();
    log_->debug("connection disconnected: host:{}, port:{}",
                inet_ntoa(addr_.sin_addr),
                ntohs(addr_.sin_port));
    status_ = ConnectionStatus_DISCONNECTED;
    return RetCode_OK;
}

RetCode connection::socket_shutdown()
{
    close(socket_);
    return RetCode_OK;
}

void connection::reset_rdn_outg_rep()
{
    pkt_ch_st_ = PktChasingStatus_BodyLen;
    rdn_buff_.reset();
    bdy_bytelen_ = 0;
    curr_rdn_body_.reset();
    while(!pkt_sending_q_.empty()) {
        pkt_sending_q_.get();
    }
    cpkt_.release();
    acc_snd_buff_.reset();
}

RetCode connection::send(const std::string &pkt)
{
    if(con_type_ == ConnectionType_TCP_INGOING ||
            con_type_ == ConnectionType_TCP_OUTGOING) {
        return send_stream(pkt);
    } else if(con_type_ == ConnectionType_UDP_OUTGOING) {
        return send_datagram(pkt);
    }
    return RetCode_KO;
}

RetCode connection::send_stream(const std::string &pkt)
{
    uint sz = (uint)pkt.size();
    g_bbuf *ppkt = new g_bbuf(sz+4);
    ppkt->append_uint(sz);
    ppkt->append(pkt.c_str(), 0, sz);
    ppkt->set_read();
    pkt_sending_q_.put(std::unique_ptr<g_bbuf>(ppkt));
    sel_.notify(new event(SendPacket, self_as_shared_ptr()));
    return RetCode_OK;
}

RetCode connection::send_datagram(const std::string &pkt)
{
    acc_snd_buff_.reset();
    uint sz = (uint)pkt.size();
    acc_snd_buff_.ensure_capacity(sz+4);
    acc_snd_buff_.append_uint(sz);
    acc_snd_buff_.append(pkt.c_str(), 0, sz);
    acc_snd_buff_.set_read();

    int nbytes = sendto(socket_,
                        &acc_snd_buff_.buf_[acc_snd_buff_.pos_],
                        acc_snd_buff_.available_read(),
                        0,
                        (struct sockaddr *) &addr_,
                        sizeof(addr_));
    if(nbytes < 0) {
        log_->error("send_datagram - errno:{}", errno);
        return RetCode_KO;
    }
    return RetCode_OK;
}

RetCode connection::send_acc_buff()
{
    acc_snd_buff_.set_read();
    if(!acc_snd_buff_.limit_) {
        return RetCode_BADARG;
    }
    RetCode rcode = RetCode_OK;
    bool stay = true;
    long bsent = 0, tot_bsent = 0, remaining = (long)acc_snd_buff_.available_read();
    while(stay) {
        while(remaining && ((bsent = ::send(socket_,
                                            &acc_snd_buff_.buf_[acc_snd_buff_.pos_],
                                            (int)remaining, 0)) > 0)) {
            acc_snd_buff_.advance_pos_read(bsent);
            tot_bsent += bsent;
            remaining -= bsent;
        }
        if(remaining) {
            if((rcode = sckt_hndl_err(bsent)) == RetCode_SCKWBLK) {
                acc_snd_buff_.set_mark();
            }
            stay = false;
            break;
        } else {
            acc_snd_buff_.reset();
            stay = false;
            break;
        }
    }
    return rcode;
}

RetCode connection::aggr_msgs_and_send_pkt()
{
    RetCode rcode = RetCode_OK;
    /*  1
        try to fill accumulating buffer with current packet and queued messages.
    */
    do {
        acc_snd_buff_.set_read();
        while(acc_snd_buff_.available_read() < acc_snd_buff_.capacity()) {
            if(cpkt_ && cpkt_->available_read()) {
                acc_snd_buff_.set_write();
                if(!acc_snd_buff_.append_no_rsz(*cpkt_)) {
                    //accumulating buffer filled.
                    break;
                } else {
                    //current packet has completely read, it can be replaced.
                }
            } else {
                if(!pkt_sending_q_.empty()) {
                    cpkt_ = pkt_sending_q_.get();
                    cpkt_->set_read();
                } else {
                    //current packet read and empty queue
                    break;
                }
            }
        }
        /*  2
            send accumulating buffer
        */
    } while(!(rcode = send_acc_buff()) && (cpkt_->available_read() || !pkt_sending_q_.empty()));
    return rcode;
}

RetCode connection::recv_bytes(char *src_ip)
{
    RetCode rcode = RetCode_OK;
    rdn_buff_.set_write();
    long brecv = 0, buf_rem_len = (long)rdn_buff_.remaining();
    struct sockaddr_in src_addr;
    socklen_t len = sizeof(src_addr);

    while(buf_rem_len && ((brecv = recvfrom(socket_,
                                            &rdn_buff_.buf_[rdn_buff_.pos_],
                                            buf_rem_len,
                                            0,
                                            (sockaddr *)&src_addr,
                                            &len)) > 0)) {
        rdn_buff_.move_pos_write(brecv);
        buf_rem_len -= brecv;
        inet_ntop(AF_INET, &src_addr.sin_addr, src_ip, 16);
    }
    if(brecv<0) {
        rcode = sckt_hndl_err(brecv);
    }
    return rcode;
}

RetCode connection::chase_pkt()
{
    RetCode rcode = RetCode_PARTPKT;
    bool pkt_rdy = false, stay = true;
    rdn_buff_.set_read();
    while(stay && rdn_buff_.available_read() && !pkt_rdy) {
        switch(pkt_ch_st_) {
            case PktChasingStatus_BodyLen:
                if(rdn_buff_.available_read() >= WORD_SZ) {
                    rdn_buff_.read_uint(&bdy_bytelen_);
                    pkt_ch_st_ = PktChasingStatus_Body;
                } else {
                    stay = false;
                }
                break;
            case PktChasingStatus_Body:
                if(rdn_buff_.available_read()) {
                    if(!curr_rdn_body_) {
                        curr_rdn_body_.reset(new g_bbuf(bdy_bytelen_));
                    }
                    rdn_buff_.read(std::min(curr_rdn_body_->remaining(), rdn_buff_.available_read()), *curr_rdn_body_);
                    if(curr_rdn_body_->remaining()) {
                        stay = false;
                    } else {
                        curr_rdn_body_->set_read();
                        pkt_ch_st_ = PktChasingStatus_BodyLen;
                        pkt_rdy = true;
                    }
                } else {
                    stay = false;
                }
                break;
            default:
                break;
        }
    }
    size_t avl_rd = rdn_buff_.available_read();
    if(avl_rd) {
        rdn_buff_.set_mark();
        if(avl_rd < WORD_SZ) {
            rdn_buff_.compact();
        }
    } else {
        rdn_buff_.reset();
    }
    return pkt_rdy ? RetCode_OK : rcode;
}

RetCode connection::recv_pkt(const char *src_ip)
{
    RetCode rcode = RetCode_OK;
    sel_.peer_.incoming_evt_q_.put(event(self_as_shared_ptr(),
                                         std::move(curr_rdn_body_),
                                         src_ip));
    return rcode;
}

void connection::on_established()
{
    if(con_type_ == ConnectionType_TCP_INGOING) {
        sel_.peer_.incoming_evt_q_.put(event(IncomingConnect, self_as_shared_ptr()));
    }
}

}
