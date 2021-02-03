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

namespace vlg {

// broker_impl
struct broker_impl {
        explicit broker_impl(broker &, broker_listener &);

        RetCode set_params_file_dir(const char *dir);

        void set_cfg_srv_sin_addr(const char *addr);
        void set_cfg_srv_sin_port(int port);


        unsigned int next_connid() {
            std::unique_lock<std::mutex> lck(mtx_);
            return ++prgr_conn_id_;
        }

        RetCode new_incoming_connection(std::shared_ptr<incoming_connection> &new_connection,
                                        unsigned int connid = 0);

        RetCode submit_inco_evt_task(std::shared_ptr<incoming_connection> &conn_sh,
                                     vlg_hdr_rec &pkt_hdr,
                                     std::unique_ptr<g_bbuf> &pkt_body);

        RetCode submit_outg_evt_task(outgoing_connection_impl *oconn,
                                     vlg_hdr_rec &pkt_hdr,
                                     std::unique_ptr<g_bbuf> &pkt_body);

        RetCode submit_sbs_evt_task(subscription_event_impl &sbs_evt,
                                    s_hm &connid_condesc_set);

    private:
        RetCode init();
        RetCode start_exec_services();

    private:
        virtual const char *get_automa_name() override;
        virtual const unsigned int *get_automa_version() override;

        virtual RetCode on_automa_load_config(int pnum,
                                              const char *param,
                                              const char *value) override;

        virtual RetCode on_automa_early_init() override;
        virtual RetCode on_automa_init() override;
        virtual RetCode on_automa_start() override;
        virtual RetCode on_automa_stop() override;
        virtual RetCode on_automa_move_running() override;
        virtual void on_automa_error() override;

    public:
        selector selector_;
};

}
