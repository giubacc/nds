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

#include <vector>
#include "gtest/gtest.h"
#include "peer.h"

int mock_main(int argc, char *argv[], nds::peer &pr);

struct peer_tester {

    int start(int argc, char *argv[]) {
        argc_ = argc;
        argv_ = argv;
        daemon_.reset(new std::thread([&]() {
            res_ = mock_main(argc_, argv_, pr_);
        }));
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return res_;
    }

    int argc_ = 0;
    char **argv_ = nullptr;
    std::unique_ptr<std::thread> daemon_;
    nds::peer pr_;
    int res_ = 0;
};

peer_tester node1;
peer_tester node2;
peer_tester setter_cli;

std::vector<const char *> node1_args = {"test", "-n", "-v", "trace", "-l", "n1"};
std::vector<const char *> node2_args = {"test", "-n", "-v", "trace", "-l", "n2"};

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);

    //start daemon nodes
    node1.start(node1_args.size(), (char **)node1_args.data());
    node2.start(node2_args.size(), (char **)node2_args.data());

    int res = RUN_ALL_TESTS();

    //stop daemon nodes
    node1.pr_.stop();
    node2.pr_.stop();
    node1.daemon_->join();
    node2.daemon_->join();

    return res;
}

//TESTS

TEST(DaemonNodesStatus, CheckInitialStatus)
{
    //pre synch nodes status
    EXPECT_EQ(node1.pr_.current_node_ts_, 0U);
    EXPECT_EQ(node2.pr_.current_node_ts_, 0U);
    EXPECT_EQ(node1.pr_.desired_cluster_ts_, 0U);
    EXPECT_EQ(node2.pr_.desired_cluster_ts_, 0U);
    EXPECT_EQ(node1.pr_.data_, "");
    EXPECT_EQ(node2.pr_.data_, "");

    //wait synch takes place
    std::this_thread::sleep_for(std::chrono::seconds(3));
    EXPECT_NE(node1.pr_.current_node_ts_, 0U);
    EXPECT_NE(node2.pr_.current_node_ts_, 0U);
    EXPECT_NE(node1.pr_.desired_cluster_ts_, 0U);
    EXPECT_NE(node2.pr_.desired_cluster_ts_, 0U);
    EXPECT_EQ(node1.pr_.current_node_ts_, node2.pr_.current_node_ts_);
    EXPECT_EQ(node1.pr_.current_node_ts_, node1.pr_.desired_cluster_ts_);
    EXPECT_EQ(node1.pr_.data_, "");
    EXPECT_EQ(node2.pr_.data_, "");
}

TEST(DaemonNodesStatus, SetValueJerico)
{
    std::vector<const char *> setter_cli_args = {"test", "set", "Jerico", "-v", "trace", "-l", "s1"};
    setter_cli.start(setter_cli_args.size(), (char **)setter_cli_args.data());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_NE(node1.pr_.current_node_ts_, 0U);
    EXPECT_NE(node2.pr_.current_node_ts_, 0U);
    EXPECT_NE(node1.pr_.desired_cluster_ts_, 0U);
    EXPECT_NE(node2.pr_.desired_cluster_ts_, 0U);
    EXPECT_EQ(node1.pr_.current_node_ts_, node2.pr_.current_node_ts_);
    EXPECT_EQ(node1.pr_.current_node_ts_, node1.pr_.desired_cluster_ts_);
    EXPECT_EQ(node1.pr_.data_, "Jerico");
    EXPECT_EQ(node2.pr_.data_, "Jerico");

    EXPECT_EQ(setter_cli.pr_.current_node_ts_, node1.pr_.current_node_ts_);
    EXPECT_EQ(setter_cli.pr_.desired_cluster_ts_, node1.pr_.desired_cluster_ts_);
    EXPECT_EQ(setter_cli.pr_.data_, "Jerico");

    setter_cli.pr_.stop();
    setter_cli.daemon_->join();
}
