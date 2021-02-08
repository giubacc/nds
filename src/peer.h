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
#include "selector.h"

namespace nds {

/** @brief a peer
*/
struct peer {

    //peer configuration
    struct cfg {
        bool start_node = false;
        std::string multicast_address = "232.232.200.82";
        uint16_t multicast_port = 8745;
        uint16_t listening_port = 31582;
        std::string val;
        bool get_val = false;

        std::string log_type = "console";
        std::string log_level = "info";

        spdlog::level::level_enum get_spdloglvl() const;
    };

    explicit peer();
    ~peer();

    int run();

    RetCode init();
    RetCode start();
    RetCode stop();
    RetCode process_incoming_events();
    RetCode process_node_status();

    bool foreign_evt(const Json::Value &json_evt);
    RetCode process_foreign_evt(event &evt, Json::Value &json_evt);

    RetCode send_alive_node_msg();
    Json::Value build_alive_node_msg() const;

    RetCode send_data_msg(connection &conn);
    Json::Value build_data_msg() const;

    RetCode send_packet(const Json::Value &pkt, connection &conn);

    uint32_t gen_ts() const;

    Json::Value evt_to_json(const event &evt);

    cfg cfg_;
    selector selector_;
    std::mutex mtx_;
    std::condition_variable cv_;
    b_qu<event> incoming_evt_q_;

    //the time point at which this node will generate itself the timestamp;
    //this will happen if no other node will respond to initial alive sent by this node.
    //not daemon nodes ("pure" setter or getter nodes) will shutdown at this time point.
    std::chrono::system_clock::time_point tp_initial_synch_window_;

    //the currently timestamp set by this node
    uint32_t current_node_ts_ = 0;

    //the desired timestamp this node would like to reach.
    //a successful synch with the cluster will transit desired_cluster_ts_ into current_node_ts_.
    uint32_t desired_cluster_ts_ = 0;

    //the value shared across the cluser
    std::string data_;

    std::shared_ptr<spdlog::logger> log_;
};

}
