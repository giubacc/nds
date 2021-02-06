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

#pragma once
#include "bbuf.h"
#include "concurr.h"

namespace nds {
struct peer;
struct selector;

enum PktChasingStatus {
    PktChasingStatus_BodyLen,
    PktChasingStatus_Body
};

// ConnectionType
enum ConnectionType {
    ConnectionType_UNDEFINED,
    ConnectionType_INGOING,             //TCP ingoing
    ConnectionType_OUTGOING,            //TCP outgoing
    ConnectionType_UDP_MCAST_INGOING,  //UDP ingoing multicast
    ConnectionType_UDP_MCAST_OUTGOING,  //UDP outgoing multicast
};

// ConnectionStatus
enum ConnectionStatus {
    ConnectionStatus_DISCONNECTED,
    ConnectionStatus_ESTABLISHED,
};

struct connection {

    connection(selector &sel, ConnectionType ct);

    const char *get_host_ip() const;
    unsigned short get_host_port() const;

    void set_host_ip(const char *ip);
    void set_host_port(unsigned short port);

    RetCode set_socket_blocking_mode(bool blocking);
    RetCode sckt_hndl_err(long sock_op_res);

    RetCode establish_multicast(sockaddr_in &params);
    RetCode establish_connection(sockaddr_in &params);

    RetCode set_connection_established();
    RetCode close_connection();
    RetCode socket_shutdown();

    void reset_rdn_outg_rep();

    //receiving, used by both TCP/UDP connections
    RetCode recv_pkt();
    RetCode recv_bytes();
    RetCode chase_pkt();
    RetCode read_decode_hdr();

    //accumulating-and-fire sending, only used by TCP connections
    RetCode send_acc_buff();
    RetCode aggr_msgs_and_send_pkt();

    //single datagram sending, only used by UDP connections
    RetCode send_datagram();

    void on_established();

    ConnectionType con_type_;
    ConnectionStatus status_;
    SOCKET socket_;
    struct sockaddr_in addr_;

    //reading rep
    PktChasingStatus pkt_ch_st_;
    unsigned int bdy_bytelen_;
    g_bbuf rdn_buff_;
    g_bbuf curr_rdn_body_;

    //packet sending queue
    b_qu<std::unique_ptr<g_bbuf>> pkt_sending_q_;
    //current sending packet
    std::unique_ptr<g_bbuf> cpkt_;
    //accumulating sending buffer
    g_bbuf acc_snd_buff_;

    std::shared_ptr<spdlog::logger> log_;
};

}
