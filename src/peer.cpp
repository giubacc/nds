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

// peer

peer::peer() :
    selector_(*this)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream os;
    os << "peer_" << ctime(&tt) << ".log";
    std::shared_ptr<spdlog::logger> lggr = spdlog::basic_logger_mt("basic_logger", os.str().c_str());
    lggr->set_level(spdlog::level::level_enum::trace);
    spdlog::register_logger(lggr);
}

peer::~peer()
{
    spdlog::shutdown();
}

RetCode peer::init()
{
    RetCode rcode = RetCode_OK;
    RET_ON_KO(selector_.init())
    return rcode;
}

// CONFIG SETTERS

void peer::set_cfg_srv_sin_addr(const char *addr)
{
    selector_.srv_sockaddr_in_.sin_addr.s_addr = inet_addr(addr);
}

void peer::set_cfg_srv_sin_port(int port)
{
    selector_.srv_sockaddr_in_.sin_port = htons(port);
}

RetCode peer::start()
{
    SelectorStatus current = SelectorStatus_UNDEF;
    RetCode rcode = RetCode_OK;

    log_->debug("[start selector thread]");
    selector_.start();
    log_->debug("[wait selector go init]");
    selector_.await_for_status_reached(SelectorStatus_INIT,
                                       current,
                                       NDS_INT_AWT_TIMEOUT,
                                       0);

    log_->debug("[request selector go ready]");
    selector_.set_status(SelectorStatus_REQUEST_READY);
    log_->debug("[wait selector go ready]");
    selector_.await_for_status_reached(SelectorStatus_READY,
                                       current,
                                       NDS_INT_AWT_TIMEOUT,
                                       0);

    log_->debug("[request selector go selecting]");
    selector_.set_status(SelectorStatus_REQUEST_SELECT);

    return rcode;
}

RetCode peer::stop()
{
    RetCode rcode = RetCode_OK;

    log_->debug("[request selector to stop]");
    selector_.set_status(SelectorStatus_REQUEST_STOP);
    selector_.interrupt();

    SelectorStatus current = SelectorStatus_UNDEF;
    selector_.await_for_status_reached(SelectorStatus_STOPPED, current);
    log_->debug("[selector stopped]");
    selector_.set_status(SelectorStatus_INIT);

    return rcode;
}

}

