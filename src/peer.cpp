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

#include "peer.h"

#define NDS_INT_AWT_TIMEOUT 1

namespace nds {

/**
 * Protocol keys (starting with:_) and fixed values.
 * Keys/values are put inside a Json message.
 *
 * An example of alive message (UDP multicast):
 *
 *      {
 *       "_lp" : 31582,
 *       "_pt" : "an",
 *       "_si" : "172.17.0.2",
 *       "_ts" : 1612981749
 *      }
 *
 * An example of Data message (TCP):
 *
 *     {
 *      "_dv" : "Jerico",
 *      "_pt" : "dt",
 *      "_ts" : 1612981862
 *     }
 *
*/
const std::string pkt_type              = "_pt";    //packet type
const std::string pkt_type_alive_node   = "an";     //packet type value: Alive Node (UDP multicast)
const std::string pkt_type_data         = "dt";     //packet type value: Data (TCP)

//packet source ip: the ip of the outgoing interface of the source host
const std::string pkt_source_ip         = "_si";

//packet source listening port: the listening port of the source host
const std::string pkt_listening_port    = "_lp";

const std::string pkt_ts                = "_ts";    //packet timestamp: the timestamp of the packet
const std::string pkt_data_value        = "_dv";    //packet data: the value inside a Data packet (TCP)

//packet interrupt: a key used to generate events inside the application (interrupts generated by selector/peer thread)
const std::string pkt_interrupt         = "_ir";

// peer

peer::peer() :
    selector_(*this)
{}

peer::~peer()
{
    spdlog::shutdown();
}

spdlog::level::level_enum peer::cfg::get_spdloglvl() const
{
    if(log_level == "trace") {
        return spdlog::level::level_enum::trace;
    } else if(log_level == "info") {
        return spdlog::level::level_enum::info;
    } else if(log_level == "warn") {
        return spdlog::level::level_enum::warn;
    } else if(log_level == "err") {
        return spdlog::level::level_enum::err;
    } else {
        return spdlog::level::level_enum::off;
    }
}

#define NODE_SYNCH_DURATION 2

RetCode peer::init()
{
    RetCode rcode = RetCode_OK;

    //logger init
    std::shared_ptr<spdlog::logger> log;
    if(cfg_.log_type == "console") {
        log = spdlog::stdout_color_mt("console");
    } else {
        log = spdlog::basic_logger_mt(cfg_.log_type, cfg_.log_type);
    }

    log->set_pattern("[%H:%M:%S:%e][%t][%^%l%$]%v");
    log->set_level(cfg_.get_spdloglvl());
    spdlog::flush_every(std::chrono::seconds(2));
    log_ = log;

    //selector init
    RET_ON_KO(selector_.init())

    //seconds before this node will auto generate the timestamp
    tp_initial_synch_window_ = std::chrono::system_clock::now() + std::chrono::duration<int>(NODE_SYNCH_DURATION);

    return rcode;
}

RetCode peer::start()
{
    SelectorStatus current = SelectorStatus_UNDEF;
    RetCode rcode = RetCode_OK;
    exit_required_ = false;

    log_->debug("start selector thread");
    selector_.start();
    log_->debug("wait selector go init");
    selector_.await_for_status_reached(SelectorStatus_INIT,
                                       current,
                                       NDS_INT_AWT_TIMEOUT,
                                       0);

    log_->debug("request selector go ready");
    selector_.set_status(SelectorStatus_REQUEST_READY);
    log_->debug("wait selector go ready");
    selector_.await_for_status_reached(SelectorStatus_READY,
                                       current,
                                       NDS_INT_AWT_TIMEOUT,
                                       0);

    log_->debug("request selector go selecting");
    selector_.set_status(SelectorStatus_REQUEST_SELECT);

    selector_.await_for_status_reached(SelectorStatus_SELECT,
                                       current,
                                       NDS_INT_AWT_TIMEOUT,
                                       0);

    log_->debug("selector is selecting");
    return rcode;
}

RetCode peer::stop()
{
    RetCode rcode = RetCode_OK;
    exit_required_ = true;

    log_->debug("request selector to stop");
    selector_.set_status(SelectorStatus_REQUEST_STOP);
    selector_.interrupt();

    SelectorStatus current = SelectorStatus_UNDEF;
    selector_.await_for_status_reached(SelectorStatus_STOPPED, current);
    log_->debug("selector stopped");
    selector_.set_status(SelectorStatus_INIT);

    return rcode;
}

RetCode peer::process_incoming_events()
{
    RetCode rcode = RetCode_OK;
    log_->debug("processing incoming events ...");

    while(true) {
        event evt = incoming_evt_q_.get();
        Json::Value json_evt = evt_to_json(evt);

        if(evt.evt_ == Interrupt) {
#if 0
            log_->trace("peer received an interrupt");
#endif
            if(process_node_status() == RetCode_EXIT) {
                break;
            }
        } else if(evt.evt_ == IncomingConnect) {
            log_->debug("sending data to node");
            send_data_msg(*evt.conn_);
        } else if((evt.evt_ == PacketAvailable) && foreign_evt(json_evt)) {
            //packet from multicast or tcp connection
            log_->trace("evt:\n{}", json_evt.toStyledString());
            if(process_foreign_evt(evt, json_evt) == RetCode_EXIT) {
                break;
            }
        } else {
            log_->debug("evt is from this node, discarding ...");
        }
    }

    return rcode;
}

RetCode peer::process_foreign_evt(event &evt, Json::Value &json_evt)
{
    RetCode rcode = RetCode_OK;
    std::string ptype = json_evt[pkt_type].asString();

    if(ptype == pkt_type_alive_node) {
        time_t oth_ts = json_evt[pkt_ts].asUInt();

        if(!current_node_ts_ && !oth_ts) {
            log_->debug("discarding alive evt from other newly spawned node: this node is still synching");
            return RetCode_OK;
        }

        if(current_node_ts_ > oth_ts) {
            if(current_node_ts_ == desired_cluster_ts_) {
                log_->debug("other node is not updated: [this_ts > other_ts], notifying it ...");
                send_alive_node_msg();
            } else {
                //this node is already synching with the cluster; do not send potentially useless alive.
            }
            return RetCode_OK;
        } else if(current_node_ts_ < oth_ts) {
            if(desired_cluster_ts_ < oth_ts) {
                desired_cluster_ts_ = oth_ts;
                log_->debug("this node is not updated: [this_ts < other_ts], requesting updated data ...");
                std::shared_ptr<connection> outg_conn(new connection(selector_, ConnectionType_TCP_OUTGOING));
                outg_conn->set_host_ip(json_evt[pkt_source_ip].asCString());
                outg_conn->set_host_port(json_evt[pkt_listening_port].asUInt());
                selector_.notify(new event(ConnectRequest, outg_conn));
            } else {
                //already requested to someone else, do nothing
            }
        } else {
            //equals, do nothing
        }

    } else if(ptype == pkt_type_data) {
        time_t data_ts = json_evt[pkt_ts].asUInt();
        if(data_ts > current_node_ts_) {
            data_ = json_evt[pkt_data_value].asString();
            current_node_ts_ = data_ts;

            if(cfg_.get_val) {
                return RetCode_EXIT;
            }
        }
        if(data_ts < desired_cluster_ts_) {
            //data received is earlier than cluster one, notify cluster
            send_alive_node_msg();
        }
        evt.conn_->close_connection();
    } else {
        log_->error("unk pkt_type: {}", ptype);
    }

    return rcode;
}

RetCode peer::process_node_status()
{
    RetCode rcode = RetCode_OK;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    if(exit_required_) {
        return RetCode_EXIT;
    }

    if(!cfg_.start_node && now > tp_initial_synch_window_) {
        //"pure" setter or getter nodes must shutdown.
        return RetCode_EXIT;
    }

    //if no other node has still responded to initial alive, the node generates itself the timestamp
    if(!current_node_ts_ && !desired_cluster_ts_ && now > tp_initial_synch_window_) {
        desired_cluster_ts_ = current_node_ts_ = gen_ts();
        log_->debug("auto generated timestamp: {}", current_node_ts_);
        send_alive_node_msg();
    }

    return rcode;
}

int peer::run()
{
    RetCode rcode = RetCode_OK;
    selector_.srv_sockaddr_in_.sin_port = htons(cfg_.listening_port);

    if((rcode = init())) {
        return rcode;
    }
    if((rcode = start())) {
        return rcode;
    }

    if(!cfg_.val.empty()) {
        data_ = cfg_.val;
        desired_cluster_ts_ = current_node_ts_ = gen_ts();
    }

    if((rcode = send_alive_node_msg())) {
        return rcode;
    }

    process_incoming_events();

    if(!exit_required_) {
        stop();
    }

#ifndef G_TEST
    std::cout << data_ << std::endl;
#endif
    return rcode;
}

bool peer::foreign_evt(const Json::Value &json_evt)
{
    return (json_evt[pkt_listening_port].asUInt() != ntohs(selector_.srv_sockaddr_in_.sin_port)) &&
           (selector_.hintfs_.find(json_evt[pkt_source_ip].asString()) == selector_.hintfs_.end());
}

RetCode peer::send_data_msg(connection &conn)
{
    Json::Value data_msg = build_data_msg();
    return send_packet(data_msg, conn);
}

Json::Value peer::build_data_msg() const
{
    Json::Value data_msg;
    data_msg[pkt_type] = pkt_type_data;
    data_msg[pkt_data_value] = data_;
    data_msg[pkt_ts] = current_node_ts_;
    return data_msg;
}

RetCode peer::send_alive_node_msg()
{
    Json::Value alive_node_msg = build_alive_node_msg();
    return send_packet(alive_node_msg, selector_.mcast_udp_outg_conn_);
}

Json::Value peer::build_alive_node_msg() const
{
    Json::Value alive_node_msg;
    alive_node_msg[pkt_type] = pkt_type_alive_node;
    alive_node_msg[pkt_listening_port] = ntohs(selector_.srv_sockaddr_in_.sin_port);
    alive_node_msg[pkt_ts] = current_node_ts_;
    return alive_node_msg;
}

RetCode peer::send_packet(const Json::Value &pkt, connection &conn)
{
    std::ostringstream os;
    os << pkt;
    return conn.send(os.str());
}

uint32_t peer::gen_ts() const
{
    return (uint32_t)::time(0);
}

Json::Value peer::evt_to_json(const event &evt)
{
    Json::Value jpkt;
    if(evt.evt_ == Interrupt) {
        jpkt[pkt_interrupt] = "";
    } else if(evt.opt_rdn_pkt_) {
        std::string pkts(evt.opt_rdn_pkt_->buf_, evt.opt_rdn_pkt_->available_read());
        std::istringstream istr(pkts);
        istr >> jpkt;
        if(jpkt[pkt_type].asString() == pkt_type_alive_node) {
            jpkt[pkt_source_ip] = evt.opt_src_ip_;
        }
    }
    return jpkt;
}


}

