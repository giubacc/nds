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
#include "connection.h"

namespace nds {
struct peer;
struct selector;

/**
 * event type
 *
 */
enum EvtType {
    Undef,
    Interrupt,              //generic interrupt
    ConnectRequest,         //request for TCP connection (peer -> selector)
    IncomingConnect,        //new incoming TCP connection (selector -> peer)
    SendPacket,             //request to send a packet (peer -> selector)
    PacketAvailable,        //foreign packet available (selector -> peer)
    Disconnect,             //connection disconnection event
};

/**
 * An event
 *
 * This class is used as message between selector/peer threads.
 *
 * It can model an interrupt (Interrupt).
 * It can be used by peer thread to request selector thread to connect to TCP (ConnectRequest).
 * It can transport an incoming packet from a multicast/unicast socket (PacketAvailable).
 * It can be used by peer thread to request selector to send a packet (SendPacket).
 * It can be used by selector to inform peer thread of a received foreign packet (PacketAvailable).
 * It can be used to manage an event of disconnection (Disconnect).
 */
struct event {
    explicit event();
    explicit event(EvtType evt);
    explicit event(EvtType evt, std::shared_ptr<connection> &conn);

    explicit event(std::shared_ptr<connection> &conn,
                   std::unique_ptr<g_bbuf> &&rdn_pkt,
                   const char *src_ip);

    EvtType evt_;
    std::shared_ptr<connection> conn_;
    std::unique_ptr<g_bbuf> opt_rdn_pkt_;
    char opt_src_ip_[16] = {0};
};

/**
 * TCP server socket acceptor
 *
 * A singleton class used as helper class by selector class.
 * It simply accept incoming TCP connections over the listening socket.
 */
struct acceptor {
    acceptor(peer &p);
    ~acceptor();

    RetCode set_sockaddr_in(sockaddr_in &serv_sockaddr_in);
    RetCode create_server_socket(SOCKET &serv_socket);
    RetCode accept(std::shared_ptr<connection> &new_connection);

    peer &peer_;
    SOCKET serv_socket_;
    sockaddr_in serv_sockaddr_in_;

    std::shared_ptr<spdlog::logger> log_;
};

/**
 * States of the selector automa.
 *
 */
enum SelectorStatus {
    SelectorStatus_UNDEF,               // zero status
    SelectorStatus_TO_INIT,             // selector early status
    SelectorStatus_INIT,                // selector is inited
    SelectorStatus_REQUEST_READY,       // <<-- asynch request made by peer thread to transit into Ready status.
    SelectorStatus_READY,               // selector has completed all init steps and it is ready to select
    SelectorStatus_REQUEST_SELECT,      // <<-- asynch request made by peer thread to transit into Selecting status.
    SelectorStatus_SELECT,              // selector is currently selecting over sockets
    SelectorStatus_REQUEST_STOP,        // <<-- asynch request made by peer thread to transit into Stopped status.
    SelectorStatus_STOPPING,            // selector is stopping
    SelectorStatus_STOPPED,             // selector is stopped and can be safely disposed
    SelectorStatus_ERROR = 500,
};

/**
 * selector
 *
 * The selector is a singleton class used to actively monitor all sockets of all
 * connections managed by this NDS node.
 * The selector monitors all internal/multicast UDP sockets alongside with all TCP sockets.
 * When a packet is read from a socket it is packed up and sent asynch to the peer thread.
 * The selector is also listening for events coming from peer thread (such as requests to send a packet over TCP).
 */
struct selector : public th {
    explicit selector(peer &);
    virtual ~selector();

    RetCode init();

    RetCode enumHostNetInterfaces();

    RetCode await_for_status_reached(SelectorStatus test,
                                     SelectorStatus &current,
                                     time_t sec = -1,
                                     long nsec = 0);

    RetCode notify(const event *);
    RetCode process_asyn_evts();
    RetCode interrupt();
    RetCode set_status(SelectorStatus);

    virtual void run() override;

    RetCode create_UDP_notify_srv_sock();
    RetCode connect_UDP_notify_cli_sock();

    bool is_still_valid_connection(const event *);
    RetCode process_inco_sock_inco_events();
    RetCode process_inco_sock_outg_events();
    RetCode process_outg_sock_inco_events();
    RetCode process_outg_sock_outg_events();

    RetCode start_conn_objs();

    void FDSET_sockets();
    void FDSET_incoming_sockets();
    void FDSET_outgoing_sockets();
    void FDSET_write_incoming_pending_sockets();
    void FDSET_write_outgoing_pending_sockets();

    RetCode consume_events();
    RetCode consume_inco_sock_events();

    RetCode conn_process_rdn_buff(std::shared_ptr<connection> &);

    RetCode add_outg_conn(event *);
    RetCode manage_disconnect_conn(event *);

    RetCode server_socket_shutdown();
    RetCode stop_and_clean();

    peer &peer_;
    SelectorStatus status_;
    fd_set read_FDs_, write_FDs_;

    int nfds_;
    int sel_res_;

    //internal UDP socket used to interrupt the select() and notify selector of new events.
    sockaddr_in udp_ntfy_sa_in_;
    SOCKET udp_ntfy_srv_socket_;
    SOCKET udp_ntfy_cli_socket_;

    //host network interfaces
    std::unordered_set<std::string> hintfs_;

    mutable std::mutex mtx_;
    mutable std::condition_variable cv_;

    //TCP incoming
    SOCKET srv_socket_;
    sockaddr_in srv_sockaddr_in_;
    acceptor srv_acceptor_;

    std::unordered_map<SOCKET, std::shared_ptr<connection>> inco_conn_map_;
    std::unordered_map<SOCKET, std::shared_ptr<connection>> wp_inco_conn_map_;

    //TCP outgoing
    std::unordered_map<SOCKET, std::shared_ptr<connection>> outg_conn_map_;
    std::unordered_map<SOCKET, std::shared_ptr<connection>> wp_outg_conn_map_;

    //UDP Multicast
    std::shared_ptr<connection> mcast_udp_inco_conn_;
    connection mcast_udp_outg_conn_;

    //logger
    std::shared_ptr<spdlog::logger> log_;
};

}
