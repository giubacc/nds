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
#include <queue>
#include <thread>

namespace spdlog {
class logger;
}

namespace nds {

/**
 * runnable interface.
*/
struct runnable {
    virtual ~runnable() {}
    virtual void run() = 0;
};

/**
 * A wrapper to std::thread
 *
*/
struct th : public runnable {
        explicit th() :
            target_(this) {
        }

        explicit th(runnable *target):
            target_(target) {
        }

        virtual ~th() = default;

        std::thread::id thread_id() const {
            return id_;
        }

        void join() {
            if(th_) {
                th_->join();
            }
        }

        void stop() {
            if(th_) {
                th_->detach();
                th_.release();
            }
        }

        void start() {
            th_.reset(new std::thread(th_run, this));
        }

        virtual void run() override {
        }

    private:
        static void th_run(th *thd) {
            std::this_thread::get_id();
            thd->target_->run();
        }

    private:
        runnable *target_;
        std::thread::id id_;
        std::unique_ptr<std::thread> th_;
};

/**
 * A very simple blocking queue
*/
template <typename T>
struct b_qu {
        b_qu() {}

        void put(T &&msg) {
            bool wasempty;
            std::unique_lock<std::mutex> lck(mtx_);
            wasempty=q_.empty();
            q_.push(std::move(msg));
            if(wasempty) {
                cv_.notify_all();
            }
        }

        T get() {
            std::unique_lock<std::mutex> lck(mtx_);
            while(q_.empty()) {
                cv_.wait(lck);
            }
            T msg = std::move(q_.front()); //bad hack :(
            q_.pop();
            return msg;
        }

        bool empty() const {
            std::unique_lock<std::mutex> lck(mtx_);
            return q_.empty();
        }

    private:
        std::queue<T> q_;
        mutable std::mutex mtx_;
        mutable std::condition_variable cv_;
};

}
