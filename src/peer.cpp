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

const std::string pkt_type              = "_pt";
const std::string pkt_type_alive_node   = "an";
const std::string pkt_type_fail_node    = "fn";
const std::string pkt_type_data         = "dt";
const std::string pkt_ip                = "_ip";
const std::string pkt_port              = "_po";
const std::string pkt_ts                = "_ts";
const std::string pkt_data              = "_dt";

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

RetCode peer::init()
{
    RetCode rcode = RetCode_OK;

    //logger init
    std::shared_ptr<spdlog::logger> log;
    if(cfg_.log_type == "console") {
        log = spdlog::stdout_color_mt("console");
    } else {
        log = spdlog::basic_logger_mt("basic_logger", cfg_.log_type);
    }
    log->set_pattern("[%H:%M:%S:%e][%t][%^%l%$]%v");
    log->set_level(cfg_.get_spdloglvl());
    log_ = log;

    //selector init
    RET_ON_KO(selector_.init())
    return rcode;
}

RetCode peer::start()
{
    SelectorStatus current = SelectorStatus_UNDEF;
    RetCode rcode = RetCode_OK;

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



    }

    return rcode;
}

RetCode peer::set()
{
    RetCode rcode = RetCode_OK;
    log_->debug("attempting set value:{} ...", cfg_.val);

    return rcode;
}

RetCode peer::get()
{
    RetCode rcode = RetCode_OK;
    log_->debug("attempting get value from cluster ...");

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

    if((rcode = send_alive_node_msg())) {
        return rcode;
    }

    if(cfg_.start_node) {
        process_incoming_events();
    } else {
        if(!cfg_.val.empty()) {
            set();
        }
        if(!cfg_.get) {
            get();
        }
        stop();
    }

    return rcode;
}

RetCode peer::send_alive_node_msg()
{
    Json::Value alive_node_msg = build_alive_node_msg();
    return send_packet(alive_node_msg, selector_.mcast_udp_outg_conn_);
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

uint32_t peer::get_ts() const
{
    return ts_;
}

void peer::set_ts(uint32_t ts)
{
    ts_ = ts;
}

Json::Value peer::evt_to_json(const event &evt)
{
    std::string pkts(evt.opt_rdn_pkt_->buf_, evt.opt_rdn_pkt_->available_read());
    std::istringstream istr(pkts);
    Json::Value jpkt;
    istr >> jpkt;
    jpkt[pkt_ip] = evt.opt_src_ip_;
    log_->debug("evt:\n{}", jpkt.toStyledString());
    return jpkt;
}

Json::Value peer::build_alive_node_msg() const
{
    Json::Value alive_node_msg;
    alive_node_msg[pkt_type] = pkt_type_alive_node;
    alive_node_msg[pkt_port] = cfg_.listening_port;
    alive_node_msg[pkt_ts] = get_ts();
    return alive_node_msg;
}

}

