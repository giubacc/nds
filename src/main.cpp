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
#include "nds.h"
#include "clipp.h"

int main(int argc, char *argv[])
{
    bool start_node = false;
    std::string multicast_address = "239.0.0.82";
    uint16_t listening_port = 31582;
    std::string val;
    bool read = false;

    auto cli = (
                   clipp::option("-j", "--join")
                   .set(start_node, true)
                   .doc("spawn a node and join the cluster") & clipp::opt_value("multicast address", multicast_address),

                   clipp::option("-p", "--port")
                   .doc("spawn a node listening on the specified port") & clipp::opt_value("listening port", listening_port),

                   clipp::opt_value("set").set(val).doc("set the value shared accross the cluster"),
                   clipp::option("read").set(read).doc("read the value shared accross the cluster")
               );

    if(!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, argv[0]);
        return 1;
    }

    return 0;
}
