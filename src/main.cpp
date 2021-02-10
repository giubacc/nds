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

#include <iostream>
#include "peer.h"
#include "clipp.h"

int main(int argc, char *argv[])
{
    nds::peer pr;

    auto cli = (
                   clipp::option("-n", "--node")
                   .set(pr.cfg_.start_node, true)
                   .set(pr.cfg_.get_val, false)
                   .doc("spawn a new node"),

                   clipp::option("-j", "--join")
                   .doc("join the cluster at specified multicast group")
                   & clipp::value("multicast address", pr.cfg_.multicast_address),

                   clipp::option("-p", "--port")
                   .doc("listen on the specified port")
                   & clipp::value("listening port", pr.cfg_.listening_port),

                   clipp::option("-l", "--log")
                   .doc("specify logging type [console (default), file name")
                   & clipp::value("logging type", pr.cfg_.log_type),

                   clipp::option("-v", "--verbosity")
                   .doc("specify logging verbosity [off, trace, info (default), warn, err")
                   & clipp::value("logging verbosity", pr.cfg_.log_level),

                   clipp::option("set")
                   .set(pr.cfg_.get_val, false)
                   .doc("set the value shared across the cluster")
                   & clipp::value("value", pr.cfg_.val),

                   clipp::option("get")
                   .set(pr.cfg_.get_val, true)
                   .doc("get the value shared across the cluster")
               );

    if(!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, argv[0]);
        return 1;
    }

    int res = pr.run();
    return res;
}
