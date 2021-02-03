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
#include "nds.h"

namespace vlg {

#if defined WIN32 && defined _MSC_VER
RetCode WSA_init(std::shared_ptr<spdlog::logger> &log);
RetCode WSA_destroy(std::shared_ptr<spdlog::logger> &log);
#endif

struct acceptor {
    acceptor(broker_impl &broker);
    ~acceptor();

    RetCode set_sockaddr_in(sockaddr_in &serv_sockaddr_in);
    RetCode create_server_socket(SOCKET &serv_socket);
    RetCode accept(unsigned int new_connid, std::shared_ptr<incoming_connection> &new_connection);

    broker_impl &broker_;
    SOCKET serv_socket_;
    sockaddr_in serv_sockaddr_in_;
};

/*****************************************
SELECTOR EVTS
******************************************/
enum VLG_SELECTOR_Evt {
    VLG_SELECTOR_Evt_Undef,
    VLG_SELECTOR_Evt_Interrupt,
    VLG_SELECTOR_Evt_SendPacket,
    VLG_SELECTOR_Evt_ConnectRequest,
    VLG_SELECTOR_Evt_Disconnect,
    VLG_SELECTOR_Evt_ConnReqAccepted,
    VLG_SELECTOR_Evt_ConnReqRefused,
    VLG_SELECTOR_Evt_Inactivity,
};

// sel_evt

struct sel_evt {
    explicit sel_evt(VLG_SELECTOR_Evt evt, conn_impl *conn);
    explicit sel_evt(VLG_SELECTOR_Evt evt, std::shared_ptr<incoming_connection> &conn);

    VLG_SELECTOR_Evt evt_;
    ConnectionType con_type_;
    conn_impl *conn_;
    std::shared_ptr<incoming_connection> inco_conn_;
    SOCKET socket_;
    sockaddr_in saddr_;
};

// SelectorStatus

enum SelectorStatus {
    SelectorStatus_UNDEF,
    SelectorStatus_TO_INIT,
    SelectorStatus_INIT,
    SelectorStatus_REQUEST_READY,    // <<-- asynch request to transit into Ready status.
    SelectorStatus_READY,
    SelectorStatus_REQUEST_SELECT,   // <<-- asynch request to transit into Selecting status.
    SelectorStatus_SELECT,
    SelectorStatus_REQUEST_STOP,     // <<-- asynch request to transit into Stopped status.
    SelectorStatus_STOPPING,
    SelectorStatus_STOPPED,
    SelectorStatus_ERROR = 500,
};

// selector

struct selector : public th {
    explicit selector(broker_impl &);
    virtual ~selector();

    RetCode init();
    RetCode on_broker_move_running_actions();

    RetCode await_for_status_reached(SelectorStatus test,
                                     SelectorStatus &current,
                                     time_t sec = -1,
                                     long nsec = 0);

    RetCode notify(const sel_evt *);
    RetCode process_asyn_evts();
    RetCode interrupt();
    RetCode set_status(SelectorStatus);
    virtual void run() override;
    RetCode create_UDP_notify_srv_sock();
    RetCode connect_UDP_notify_cli_sock();
    bool is_still_valid_connection(const sel_evt *);
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
    RetCode server_socket_shutdown();
    RetCode consume_events();
    RetCode consume_inco_sock_events();
    RetCode add_early_outg_conn(sel_evt *);
    RetCode promote_early_outg_conn(outgoing_connection_impl *);
    RetCode delete_early_outg_conn(outgoing_connection_impl *);
    RetCode manage_disconnect_conn(sel_evt *);
    RetCode stop_and_clean();
    RetCode inco_conn_process_rdn_buff(std::shared_ptr<incoming_connection> &);
    RetCode outg_conn_process_rdn_buff(outgoing_connection_impl *);

    //rep
    broker_impl &broker_;
    SelectorStatus status_;
    fd_set read_FDs_, write_FDs_;

    int nfds_;
    int sel_res_;
    timeval sel_timeout_;
    sockaddr_in udp_ntfy_sa_in_;
    SOCKET udp_ntfy_srv_socket_;
    SOCKET udp_ntfy_cli_socket_;

    mutable std::mutex mtx_;
    mutable std::condition_variable cv_;

    //incoming
    SOCKET srv_socket_;
    sockaddr_in srv_sockaddr_in_;
    acceptor srv_acceptor_;
    std::unordered_map<uint64_t, std::shared_ptr<incoming_connection>> inco_conn_map_;
    std::unordered_map<uint64_t, std::shared_ptr<incoming_connection>> wp_inco_conn_map_;

    //outgoing
    std::unordered_map<uint64_t, outgoing_connection_impl *> outg_conn_map_;
    std::unordered_map<uint64_t, outgoing_connection_impl *> wp_outg_conn_map_;
};

}
