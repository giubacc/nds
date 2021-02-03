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
    con_type_(ct),
    status_(ConnectionStatus_DISCONNECTED),
    socket_(INVALID_SOCKET),
    pkt_ch_st_(PktChasingStatus_BodyLen),
    bdy_bytelen_(0),
    rdn_buff_(RCV_SND_BUF_SZ),
    acc_snd_buff_(RCV_SND_BUF_SZ),
    log_(sel.log_)
{}

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
        int last_socket_err = errno;
        if(last_socket_err == EAGAIN || last_socket_err == EWOULDBLOCK) {
            rcode = RetCode_SCKWBLK;
        } else if(last_socket_err == ECONNRESET) {
            rcode = RetCode_SCKCLO;
        } else {
            perror(__func__);
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

RetCode connection::establish_connection(sockaddr_in &params)
{
    RetCode rcode = RetCode_OK;
    int connect_res = 0;
    if((socket_ = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
        socklen_t len = sizeof(sockaddr_in);
        if((connect_res = connect(socket_, (struct sockaddr *)&params, len)) != INVALID_SOCKET) {
        } else {
            //int last_socket_err_ = errno;
        }
    } else {
        //int last_socket_err_ = errno;
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
    status_ = ConnectionStatus_ESTABLISHED;
    return RetCode_OK;
}

RetCode connection::close_connection()
{
    socket_shutdown();
    reset_rdn_outg_rep();
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
        pkt_sending_q_.pop_msg();
    }
    cpkt_.release();
    acc_snd_buff_.reset();
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
        while(remaining && ((bsent = send(socket_,
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
                    cpkt_ = pkt_sending_q_.pop_msg();
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

RetCode connection::recv_bytes()
{
    RetCode rcode = RetCode_OK;
    rdn_buff_.set_write();
    long brecv = 0, buf_rem_len = (long)rdn_buff_.remaining();
    while(buf_rem_len && ((brecv = recv(socket_,
                                        &rdn_buff_.buf_[rdn_buff_.pos_],
                                        buf_rem_len, 0)) > 0)) {
        rdn_buff_.move_pos_write(brecv);
        buf_rem_len -= brecv;
    }
    if(brecv<=0) {
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
                    if(!curr_rdn_body_.position()) {
                        curr_rdn_body_.ensure_capacity(bdy_bytelen_);
                    }
                    rdn_buff_.read(std::min(curr_rdn_body_.remaining(), rdn_buff_.available_read()), curr_rdn_body_);
                    if(curr_rdn_body_.remaining()) {
                        stay = false;
                    } else {
                        curr_rdn_body_.set_read();
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

RetCode connection::recv_pkt()
{
    RetCode rcode = RetCode_OK;

    //@todo

    return rcode;
}

//acceptor

acceptor::acceptor(peer &p) :
    peer_(p),
    serv_socket_(INVALID_SOCKET),
    log_(p.log_)
{
    memset(&serv_sockaddr_in_, 0, sizeof(serv_sockaddr_in_));
}

acceptor::~acceptor()
{
    if(serv_socket_ != INVALID_SOCKET) {
        if(close(serv_socket_) == SOCKET_ERROR) {
            log_->error("closesocket KO");
        }
    }
}

RetCode acceptor::set_sockaddr_in(sockaddr_in &serv_sockaddr_in)
{
    serv_sockaddr_in_ = serv_sockaddr_in;
    return RetCode_OK;
}

RetCode acceptor::create_server_socket(SOCKET &serv_socket)
{
    log_->info("[interface:{}, port:{}]",
               inet_ntoa(serv_sockaddr_in_.sin_addr),
               ntohs(serv_sockaddr_in_.sin_port));

    if((serv_socket = serv_socket_ = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
        log_->debug("[socket:{}][OK]", serv_socket);
        if(!bind(serv_socket_, (sockaddr *)&serv_sockaddr_in_, sizeof(sockaddr_in))) {
            log_->debug("[bind OK]");
            if(!listen(serv_socket_, SOMAXCONN)) {
                log_->debug("[listen OK]");
            } else {
                log_->error("[listen KO]");
                return RetCode_SYSERR;
            }
        } else {
            int err = 0;
            err = errno;
            log_->error("[bind KO][err:{}]", err);
            return RetCode_SYSERR;
        }
    } else {
        log_->error("[socket KO]");
        return RetCode_SYSERR;
    }
    return RetCode_OK;
}

RetCode acceptor::accept(std::shared_ptr<connection> &new_connection)
{
    SOCKET socket = INVALID_SOCKET;
    struct sockaddr_in addr;
    socklen_t len = 0;

    memset(&addr, 0, sizeof(addr));
    if((socket = ::accept(serv_socket_, (sockaddr *)&addr, &len)) == INVALID_SOCKET) {
        int err = 0;
        err = errno;
        log_->error("[accept KO][err:{}]", err);
        return RetCode_SYSERR;
    } else {
        log_->debug("[socket:{}, host:{}, port:{}][accept OK]",
                    socket,
                    inet_ntoa(addr.sin_addr),
                    ntohs(addr.sin_port));
    }

    new_connection->con_type_ = ConnectionType_INGOING;
    new_connection->socket_ = socket;
    new_connection->addr_ = addr;
    return RetCode_OK;
}

}
