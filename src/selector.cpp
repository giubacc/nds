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

namespace nds {

//SEL_EVT

sel_evt::sel_evt(NDS_SELECTOR_Evt evt, connection *conn) :
    evt_(evt),
    con_type_(conn ? conn->con_type_ : ConnectionType_UNDEFINED),
    conn_(conn),
    socket_(conn ? conn->socket_ : INVALID_SOCKET)
{
    memset(&saddr_, 0, sizeof(sockaddr_in));
}

sel_evt::sel_evt(NDS_SELECTOR_Evt evt, std::shared_ptr<connection> &conn) :
    evt_(evt),
    con_type_(ConnectionType_INGOING),
    conn_(conn.get()),
    inco_conn_(conn),
    socket_(conn ? conn->socket_ : INVALID_SOCKET)
{
    memset(&saddr_, 0, sizeof(sockaddr_in));
}

selector::selector(peer &p) :
    peer_(p),
    status_(SelectorStatus_TO_INIT),
    nfds_(-1),
    sel_res_(-1),
    udp_ntfy_srv_socket_(INVALID_SOCKET),
    udp_ntfy_cli_socket_(INVALID_SOCKET),
    srv_socket_(INVALID_SOCKET),
    srv_acceptor_(p)
{
    memset(&udp_ntfy_sa_in_, 0, sizeof(udp_ntfy_sa_in_));
    udp_ntfy_sa_in_.sin_family = AF_INET;
    udp_ntfy_sa_in_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    memset(&srv_sockaddr_in_, 0, sizeof(srv_sockaddr_in_));
    srv_sockaddr_in_.sin_family = AF_INET;
    srv_sockaddr_in_.sin_addr.s_addr = INADDR_ANY;
    FD_ZERO(&read_FDs_);
    FD_ZERO(&write_FDs_);
    sel_timeout_.tv_sec = 0;
    sel_timeout_.tv_usec = 0;
}

selector::~selector()
{}

RetCode selector::init()
{
    log_ = peer_.log_;
    RET_ON_KO(srv_acceptor_.set_sockaddr_in(srv_sockaddr_in_))
    RET_ON_KO(create_UDP_notify_srv_sock())
    RET_ON_KO(connect_UDP_notify_cli_sock())
    set_status(SelectorStatus_INIT);
    return RetCode_OK;
}

RetCode selector::on_broker_move_running_actions()
{
    return start_conn_objs();
}

RetCode selector::set_status(SelectorStatus status)
{
    std::unique_lock<std::mutex> lck(mtx_);
    status_ = status;
    cv_.notify_all();
    return RetCode_OK;
}

RetCode selector::await_for_status_reached(SelectorStatus test,
                                           SelectorStatus &current,
                                           time_t sec,
                                           long nsec)
{
    RetCode rcode = RetCode_OK;
    std::unique_lock<std::mutex> lck(mtx_);
    if(status_ < SelectorStatus_INIT) {
        return RetCode_BADSTTS;
    }
    if(sec<0) {
        cv_.wait(lck, [&]() {
            return status_ >= test;
        });
    } else {
        rcode = cv_.wait_for(lck, std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec), [&]() {
            return status_ >= test;
        }) ? RetCode_OK : RetCode_TIMEOUT;
    }
    current = status_;
    return rcode;
}

RetCode selector::create_UDP_notify_srv_sock()
{
    int res = 0;
    if((udp_ntfy_srv_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != INVALID_SOCKET) {
        log_->debug("[udp_ntfy_srv_socket_:{}][OK]", udp_ntfy_srv_socket_);
        if(!bind(udp_ntfy_srv_socket_, (sockaddr *)&udp_ntfy_sa_in_, sizeof(udp_ntfy_sa_in_))) {
            log_->trace("[udp_ntfy_srv_socket_:{}][bind OK]", udp_ntfy_srv_socket_);
            int flags = fcntl(udp_ntfy_srv_socket_, F_GETFL, 0);
            if(flags < 0) {
                return RetCode_KO;
            }
            flags = (flags|O_NONBLOCK);
            if((res = fcntl(udp_ntfy_srv_socket_, F_SETFL, flags))) {
                log_->critical("[udp_ntfy_srv_socket_:{}][fcntl KO][err:{}]", udp_ntfy_srv_socket_, errno);
                return RetCode_SYSERR;
            } else {
                log_->trace("[udp_ntfy_srv_socket_:{}][fcntl OK]", udp_ntfy_srv_socket_);
            }
        } else {
            log_->critical("[udp_ntfy_srv_socket_:{}][bind KO][err:{}]", udp_ntfy_srv_socket_, errno);
            return RetCode_SYSERR;
        }
    } else {
        log_->critical("[socket KO][err:{}]", errno);
        return RetCode_SYSERR;
    }
    return RetCode_OK;
}

RetCode selector::connect_UDP_notify_cli_sock()
{
    int err = 0;
    socklen_t len = sizeof(udp_ntfy_sa_in_);
    getsockname(udp_ntfy_srv_socket_, (struct sockaddr *)&udp_ntfy_sa_in_, &len);
    log_->trace("[sin_addr:{}, sin_port:{}]",
                inet_ntoa(udp_ntfy_sa_in_.sin_addr),
                htons(udp_ntfy_sa_in_.sin_port));
    if((udp_ntfy_cli_socket_ = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
        log_->debug("[udp_ntfy_cli_socket_:{}][OK]", udp_ntfy_cli_socket_);
        if((connect(udp_ntfy_cli_socket_, (struct sockaddr *)&udp_ntfy_sa_in_, sizeof(udp_ntfy_sa_in_))) != INVALID_SOCKET) {
            log_->debug("[udp_ntfy_cli_socket_:{}][connect OK]", udp_ntfy_cli_socket_);
        } else {
            log_->critical("[udp_ntfy_cli_socket_:{}][connect KO][err:{}]", udp_ntfy_cli_socket_, err);
            return RetCode_SYSERR;
        }
    } else {
        log_->critical("[socket KO]");
        return RetCode_SYSERR;
    }
    return RetCode_OK;
}

RetCode selector::interrupt()
{
    sel_evt *interrupt = new sel_evt(NDS_SELECTOR_Evt_Interrupt, nullptr);
    return notify(interrupt);
}

RetCode selector::notify(const sel_evt *evt)
{
    long bsent = 0;
    while((bsent = send(udp_ntfy_cli_socket_, (const char *)&evt, sizeof(void *), 0)) == SOCKET_ERROR) {
        int err = 0;
        err = errno;
        if(err == EAGAIN || err == EWOULDBLOCK) {
            //ok we can go ahead
        } else if(err == ECONNRESET) {
            log_->error("[udp_ntfy_cli_socket_:{}][err:{}]",  udp_ntfy_cli_socket_, err);
            return RetCode_KO;
        } else {
            perror(__func__);
            log_->error("[udp_ntfy_cli_socket_:{}][errno:{}]",  udp_ntfy_cli_socket_, errno);
            return RetCode_SYSERR;
        }
    }
    return RetCode_OK;
}

RetCode selector::start_conn_objs()
{
    RetCode res = RetCode_OK;
    if((res = srv_acceptor_.create_server_socket(srv_socket_))) {
        log_->critical("[starting acceptor, last_err:{}]",  res);
        return RetCode_KO;
    }
    FD_SET(srv_socket_, &read_FDs_);
    nfds_ = (int)srv_socket_;
    FD_SET(udp_ntfy_srv_socket_, &read_FDs_);
    nfds_ = ((int)udp_ntfy_srv_socket_ > nfds_) ? (int)udp_ntfy_srv_socket_ : nfds_;
    return res;
}

RetCode selector::process_inco_sock_inco_events()
{
    SOCKET sckt = INVALID_SOCKET;
    if(sel_res_) {
        for(auto it = inco_conn_map_.begin(); it != inco_conn_map_.end(); it++) {
            sckt = it->second->socket_;
            //first: check if it is readable.
            if(FD_ISSET(sckt, &read_FDs_)) {
                inco_conn_process_rdn_buff(it->second);
                if(!(--sel_res_)) {
                    break;
                }
            }
        }
    }
    return RetCode_OK;
}

inline RetCode selector::process_inco_sock_outg_events()
{
    for(auto it = wp_inco_conn_map_.begin(); it != wp_inco_conn_map_.end(); it++) {
        if(FD_ISSET(it->second->socket_, &write_FDs_)) {
            it->second->aggr_msgs_and_send_pkt();
            if(!(--sel_res_)) {
                break;
            }
        }
    }
    return RetCode_OK;
}

inline RetCode selector::process_outg_sock_outg_events()
{
    for(auto it = wp_outg_conn_map_.begin(); it != wp_outg_conn_map_.end(); it++) {
        if(FD_ISSET(it->second->socket_, &write_FDs_)) {
            it->second->aggr_msgs_and_send_pkt();
            if(!(--sel_res_)) {
                break;
            }
        }
    }
    return RetCode_OK;
}

RetCode selector::process_outg_sock_inco_events()
{
    if(sel_res_) {
        for(auto it = outg_conn_map_.begin(); it != outg_conn_map_.end(); it++) {
            //first: check if it is readable.
            if(FD_ISSET(it->second->socket_, &read_FDs_)) {
                outg_conn_process_rdn_buff(it->second);
                if(!(--sel_res_)) {
                    break;
                }
            }
        }
    }
    return RetCode_OK;
}

RetCode selector::consume_inco_sock_events()
{
    std::shared_ptr<connection> new_conn_shp;
    if(FD_ISSET(srv_socket_, &read_FDs_)) {
        if(srv_acceptor_.accept(new_conn_shp)) {
            log_->critical("[accepting new connection]");
            return RetCode_KO;
        }
        if(new_conn_shp->set_socket_blocking_mode(false)) {
            log_->critical("[set socket not blocking]");
            return RetCode_KO;
        }
        inco_conn_map_[new_conn_shp->socket_] = new_conn_shp;
        log_->debug("[socket:{}, host:{}, port:{}][socket accepted]",
                    new_conn_shp->socket_,
                    new_conn_shp->get_host_ip(),
                    new_conn_shp->get_host_port());
        --sel_res_;
        if(sel_res_) {
            if(process_inco_sock_inco_events()) {
                log_->critical("[processing incoming socket events]");
                return RetCode_KO;
            }
        }
    } else {
        if(process_inco_sock_inco_events()) {
            log_->critical("[processing incoming socket events]");
            return RetCode_KO;
        }
    }
    return RetCode_OK;
}

inline void selector::FDSET_sockets()
{
    FD_ZERO(&read_FDs_);
    FD_ZERO(&write_FDs_);
    FDSET_incoming_sockets();
    FDSET_outgoing_sockets();
    FDSET_write_incoming_pending_sockets();
    FDSET_write_outgoing_pending_sockets();
    FD_SET(udp_ntfy_srv_socket_, &read_FDs_);
    nfds_ = ((int)udp_ntfy_srv_socket_ > nfds_) ? (int)udp_ntfy_srv_socket_ : nfds_;
}

inline void selector::FDSET_write_incoming_pending_sockets()
{
    auto it = wp_inco_conn_map_.begin();
    while(it != wp_inco_conn_map_.end()) {
        if(it->second->acc_snd_buff_.available_read()) {
            FD_SET(it->second->socket_, &write_FDs_);
            nfds_ = ((int)it->second->socket_ > nfds_) ? (int)it->second->socket_ : nfds_;
            it++;
        } else {
            it = wp_inco_conn_map_.erase(it);
        }
    }
}

inline void selector::FDSET_write_outgoing_pending_sockets()
{
    auto it = wp_outg_conn_map_.begin();
    while(it != wp_outg_conn_map_.end()) {
        if(it->second->acc_snd_buff_.available_read()) {
            FD_SET(it->second->socket_, &write_FDs_);
            nfds_ = ((int)it->second->socket_ > nfds_) ? (int)it->second->socket_ : nfds_;
            it++;
        } else {
            it = wp_outg_conn_map_.erase(it);
        }
    }
}

inline void selector::FDSET_incoming_sockets()
{
    auto it = inco_conn_map_.begin();
    while(it != inco_conn_map_.end()) {
        if(it->second->status_ == ConnectionStatus_ESTABLISHED) {
            SOCKET inco_sock = it->second->socket_;
            FD_SET(inco_sock, &read_FDs_);
            nfds_ = ((int)inco_sock > nfds_) ? (int)inco_sock : nfds_;
            it++;
        } else {
            SOCKET inco_sock = it->second->socket_;
            wp_inco_conn_map_.erase(inco_sock);
            it = inco_conn_map_.erase(it);
        }
    }
    //always set server socket in read fds.
    FD_SET(srv_socket_, &read_FDs_);
    nfds_ = ((int)srv_socket_ > nfds_) ? (int)srv_socket_ : nfds_;
}

inline void selector::FDSET_outgoing_sockets()
{
    auto it_2 = outg_conn_map_.begin();
    while(it_2 != outg_conn_map_.end()) {
        if(it_2->second->status_ == ConnectionStatus_ESTABLISHED) {
            FD_SET(it_2->second->socket_, &read_FDs_);
            nfds_ = ((int)it_2->second->socket_ > nfds_) ? (int)it_2->second->socket_ : nfds_;
            it_2++;
        } else {
            wp_outg_conn_map_.erase(it_2->second->socket_);
            it_2 = outg_conn_map_.erase(it_2);
        }
    }
}

inline RetCode selector::manage_disconnect_conn(sel_evt *conn_evt)
{
    return RetCode_OK;
}

inline RetCode selector::add_outg_conn(sel_evt *conn_evt)
{
    RetCode rcode = RetCode_OK;
    if((rcode = conn_evt->conn_->establish_connection(conn_evt->saddr_))) {
        return rcode;
    }
    if(conn_evt->conn_->set_socket_blocking_mode(false)) {
        log_->critical("[setting socket not blocking]");
        return RetCode_KO;
    }
    outg_conn_map_[conn_evt->conn_->socket_] = (connection *)conn_evt->conn_;
    return RetCode_OK;
}

inline bool selector::is_still_valid_connection(const sel_evt *evt)
{
    if(evt->con_type_ == ConnectionType_INGOING) {
        return (inco_conn_map_.find(evt->conn_->socket_) != inco_conn_map_.end());
    } else {
        return (outg_conn_map_.find(evt->conn_->socket_) != outg_conn_map_.end());
    }
}

RetCode selector::process_asyn_evts()
{
    long brecv = 0, recv_buf_sz = sizeof(void *);
    sel_evt *conn_evt = nullptr;
    bool conn_still_valid = true;
    while((brecv = recv(udp_ntfy_srv_socket_, (char *)&conn_evt, recv_buf_sz, 0)) > 0) {
        if(brecv != recv_buf_sz) {
            log_->critical("[brecv != recv_buf_sz]");
            return RetCode_GENERR;
        }
        if(conn_evt->evt_ != NDS_SELECTOR_Evt_Interrupt) {
            /*check if we still have connection*/
            if(conn_evt->evt_ != NDS_SELECTOR_Evt_ConnectRequest) {
                conn_still_valid = is_still_valid_connection(conn_evt);
            }
            if(conn_still_valid) {
                switch(conn_evt->evt_) {
                    case NDS_SELECTOR_Evt_SendPacket:
                        if(conn_evt->con_type_ == ConnectionType_INGOING) {
                            wp_inco_conn_map_[conn_evt->socket_] = conn_evt->inco_conn_;
                        } else {
                            wp_outg_conn_map_[conn_evt->socket_] = (connection *)conn_evt->conn_;
                        }
                        break;
                    case NDS_SELECTOR_Evt_ConnectRequest:
                        add_outg_conn(conn_evt);
                        break;
                    case NDS_SELECTOR_Evt_Disconnect:
                        manage_disconnect_conn(conn_evt);
                        break;
                    default:
                        log_->critical("[unknown event]");
                        break;
                }
            } else {
                log_->info("[socket:{} is no longer valid]", conn_evt->socket_);
            }
        }
        delete conn_evt;
    }
    if(brecv == SOCKET_ERROR) {
        int err = 0;
        err = errno;
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            //ok we can go ahead
        } else if(errno == ECONNRESET) {
            log_->error("[err:{}]", err);
            return RetCode_KO;
        } else {
            perror(__func__);
            log_->critical("[errno:{}]", errno);
            return RetCode_SYSERR;
        }
    }
    if(!brecv) {
        return RetCode_KO;
    }
    return RetCode_OK;
}

inline RetCode selector::consume_events()
{
    if(FD_ISSET(udp_ntfy_srv_socket_, &read_FDs_)) {
        process_asyn_evts();
        sel_res_--;
    }
    if(sel_res_) {
        consume_inco_sock_events();
        if(sel_res_) {
            process_outg_sock_inco_events();
        }
    }
    if(sel_res_) {
        process_inco_sock_outg_events();
        if(sel_res_) {
            process_outg_sock_outg_events();
        }
    }
    return RetCode_OK;
}

RetCode selector::server_socket_shutdown()
{
    int last_err_ = 0;
    if((last_err_ = close(srv_socket_))) {
        log_->error("[socket:{}][closesocket KO][res:{}]", srv_socket_, last_err_);
    } else {
        log_->trace("[socket:{}][closesocket OK][res:{}]", srv_socket_, last_err_);
    }
    return RetCode_OK;
}

RetCode selector::stop_and_clean()
{
    for(auto it = inco_conn_map_.begin(); it != inco_conn_map_.end(); ++it)
        if(it->second->status_ != ConnectionStatus_DISCONNECTED) {
            it->second->close_connection();
        }
    wp_inco_conn_map_.clear();
    inco_conn_map_.clear();
    server_socket_shutdown();

    for(auto it = outg_conn_map_.begin(); it != outg_conn_map_.end(); ++it)
        if(it->second->status_ != ConnectionStatus_DISCONNECTED) {
            it->second->close_connection();
        }
    outg_conn_map_.clear();

    FD_ZERO(&read_FDs_);
    FD_ZERO(&write_FDs_);
    return RetCode_OK;
}

RetCode selector::inco_conn_process_rdn_buff(std::shared_ptr<connection> &ic)
{
    RetCode rcode = ic->recv_bytes();
    while(!(rcode = ic->chase_pkt())) {
        ic->recv_pkt();
    }
    return rcode;
}

RetCode selector::outg_conn_process_rdn_buff(connection *oci)
{
    RetCode rcode = oci->recv_bytes();
    while(!(rcode = oci->chase_pkt())) {
        oci->recv_pkt();
    }
    return rcode;
}

void selector::run()
{
    SelectorStatus current = SelectorStatus_UNDEF;
    if(status_ != SelectorStatus_INIT && status_ != SelectorStatus_REQUEST_READY) {
        log_->error("[status_={}, exp:2][BAD STATUS]",  status_);
    }
    do {
        log_->debug("+wait for go-ready request+");
        await_for_status_reached(SelectorStatus_REQUEST_READY, current);
        log_->debug("+go ready requested, going ready+");
        set_status(SelectorStatus_READY);
        log_->debug("+wait for go-select request+");
        await_for_status_reached(SelectorStatus_REQUEST_SELECT, current);
        log_->debug("+go-select requested, going select+");
        set_status(SelectorStatus_SELECT);
        timeval sel_timeout = sel_timeout_;
        while(status_ == SelectorStatus_SELECT) {
            if((sel_res_ = select(nfds_+1, &read_FDs_, &write_FDs_, 0, 0)) > 0) {
                consume_events();
            } else if(!sel_res_) {
                //timeout
                log_->trace("+select() [timeout]+");
            } else {
                //error
                log_->error("+select() [error:{}]+",  sel_res_);
                set_status(SelectorStatus_ERROR);
            }
            FDSET_sockets();
            sel_timeout = sel_timeout_;
        }
        if(status_ == SelectorStatus_REQUEST_STOP) {
            log_->debug("+stop requested, clean initiated+");
            stop_and_clean();
            break;
        }
        if(status_ == SelectorStatus_ERROR) {
            log_->error("+error occurred, clean initiated+");
            stop_and_clean();
            break;
        }
    } while(true);
    set_status(SelectorStatus_STOPPED);
    stop();
}

}
