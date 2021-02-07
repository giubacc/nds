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
        bool get = false;

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

    RetCode set();
    RetCode get();

    bool foreign_evt(const Json::Value &json_evt);
    RetCode process_foreign_evt(Json::Value &json_evt);

    RetCode send_alive_node_msg();
    Json::Value build_alive_node_msg() const;

    RetCode send_packet(const Json::Value &pkt, connection &conn);

    uint32_t gen_ts() const;
    uint32_t get_ts() const;
    void set_ts(uint32_t ts);

    Json::Value evt_to_json(const event &evt);

    cfg cfg_;
    selector selector_;
    std::mutex mtx_;
    std::condition_variable cv_;
    b_qu<event> incoming_evt_q_;

    //the time point at which this node will generate itself the timestamp;
    //this will happen if no other node will respond to initial alive sent by
    //this node.
    std::chrono::system_clock::time_point tp_auto_gen_ts_;

    //the timestamp shared in the cluser
    uint32_t ts_ = 0;

    std::shared_ptr<spdlog::logger> log_;
};

}
