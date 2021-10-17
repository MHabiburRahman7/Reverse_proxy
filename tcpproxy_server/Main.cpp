#include "tcpproxy_server.h"

int main(int argc, char* argv[])
{
    ////[RELEASE MODE]
    if (argc != 5)
    {
        std::cerr << "usage: tcpproxy_server <local host ip> <local port> <forward host ip> <forward port>" << std::endl;
        return 1;
    }

    const unsigned short local_port = static_cast<unsigned short>(::atoi(argv[2]));
    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[4]));
    const std::string local_host = argv[1];
    const std::string forward_host = argv[3];

    ////[DEBUG MODE]
    //const char* input_local_port = "80";
    //const char* input_forward_port = "80";
    //const char* input_local_host = "127.0.1.1";
    //const char* input_forward_host = "192.168.1.103";
    ////const char* input_forward_host = "172.217.174.110";

    //const unsigned short local_port = static_cast<unsigned short>(::atoi(input_local_port));
    //const unsigned short forward_port = static_cast<unsigned short>(::atoi(input_forward_port));
    //const std::string local_host = input_local_host;
    //const std::string forward_host = input_forward_host;

    boost::asio::io_service ios;

    try
    {
        tcp_proxy::bridge::acceptor acceptor(ios,
            local_host, local_port,
            forward_host, forward_port);

        acceptor.accept_connections();

        ios.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * [Note] On posix systems the tcp proxy server build command is as follows:
 * c++ -pedantic -ansi -Wall -Werror -O3 -o tcpproxy_server tcpproxy_server.cpp -L/usr/lib -lstdc++ -lpthread -lboost_thread -lboost_system
 */