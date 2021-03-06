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

/**
 * The status of a decoding phase when reading from a socket.
 *
 * 1. chasing body len (4 bytes)
 * 2. chasing body (n bytes)
*/
enum PktChasingStatus {
    PktChasingStatus_BodyLen,
    PktChasingStatus_Body
};

/**
 * A connection type
 */
enum ConnectionType {
    ConnectionType_UNDEFINED,
    ConnectionType_TCP_INGOING,     //TCP ingoing
    ConnectionType_TCP_OUTGOING,    //TCP outgoing
    ConnectionType_UDP_INGOING,     //UDP ingoing multicast
    ConnectionType_UDP_OUTGOING,    //UDP outgoing multicast
};

/**
 * A connection status
 */
enum ConnectionStatus {
    ConnectionStatus_DISCONNECTED,
    ConnectionStatus_ESTABLISHED,
};

/**
 * A high-level abstraction of a UDP/TCP connection.
 * It provides methods for read/write from the associated socket.
 */
struct connection {

    connection(selector &sel, ConnectionType ct);

    std::shared_ptr<connection> &self_as_shared_ptr();

    const char *get_host_ip() const;
    unsigned short get_host_port() const;

    void set_host_ip(const char *ip);
    void set_host_port(unsigned short port);

    RetCode set_socket_blocking_mode(bool blocking);
    RetCode sckt_hndl_err(long sock_op_res);

    RetCode establish_multicast(sockaddr_in &params);

    RetCode establish_connection();
    RetCode establish_connection(sockaddr_in &params);

    RetCode set_connection_established();
    RetCode close_connection();
    RetCode socket_shutdown();

    void reset_rdn_outg_rep();

    //receiving, used by both TCP/UDP connections
    RetCode recv_bytes(char *src_ip);
    RetCode recv_pkt(const char *src_ip);
    RetCode chase_pkt();
    RetCode read_decode_hdr();

    //accumulating-and-fire sending, only used by TCP connections
    RetCode send_acc_buff();
    RetCode aggr_msgs_and_send_pkt();

    //send a string as a packet
    RetCode send(const std::string &pkt);

    //send as stream, only used by TCP connections
    RetCode send_stream(const std::string &pkt);

    //single datagram sending, only used by UDP connections
    RetCode send_datagram(const std::string &pkt);

    void on_established();

    //parent
    selector &sel_;

    ConnectionType con_type_;
    ConnectionStatus status_;
    SOCKET socket_;
    struct sockaddr_in addr_;

    //reading rep
    PktChasingStatus pkt_ch_st_;
    unsigned int bdy_bytelen_;
    g_bbuf rdn_buff_;
    std::unique_ptr<g_bbuf> curr_rdn_body_;

    //packet sending queue
    b_qu<std::unique_ptr<g_bbuf>> pkt_sending_q_;
    //current sending packet
    std::unique_ptr<g_bbuf> cpkt_;
    //accumulating sending buffer
    g_bbuf acc_snd_buff_;

    //logger
    std::shared_ptr<spdlog::logger> log_;
};

}
