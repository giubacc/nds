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
        std::string node_name = "GendoIkari";
        std::string multicast_address = "232.232.200.82";
        uint16_t listening_port = 31582;
        std::string val;
        bool get = false;

        std::string log_type = "console";
        std::string log_level = "info";

        spdlog::level::level_enum get_spdloglvl() const;
    };

    explicit peer();
    ~peer();

    void set_cfg_srv_sin_addr(const char *addr);
    void set_cfg_srv_sin_port(int port);

    int run();

    RetCode init();
    RetCode start();
    RetCode stop();
    RetCode wait();

    RetCode set();
    RetCode get();

    cfg cfg_;
    selector selector_;

    std::mutex mtx_;
    std::condition_variable cv_;

    std::shared_ptr<spdlog::logger> log_;
};

}
