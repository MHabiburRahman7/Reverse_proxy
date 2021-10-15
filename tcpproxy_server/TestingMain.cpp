//
// tcpproxy_server.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2007 Arash Partow (http://www.partow.net)
// URL: http://www.partow.net/programming/tcpproxy/index.html
//
// Distributed under the Boost Software License, Version 1.0.
//
//
// Description
// ~~~~~~~~~~~
// The  objective of  the TCP  proxy server  is to  act  as  an
// intermediary  in order  to 'forward'  TCP based  connections
// from external clients onto a singular remote server.
//
// The communication flow in  the direction from the  client to
// the proxy to the server is called the upstream flow, and the
// communication flow in the  direction from the server  to the
// proxy  to  the  client   is  called  the  downstream   flow.
// Furthermore  the   up  and   down  stream   connections  are
// consolidated into a single concept known as a bridge.
//
// In the event  either the downstream  or upstream end  points
// disconnect, the proxy server will proceed to disconnect  the
// other  end  point  and  eventually  destroy  the  associated
// bridge.
//
// The following is a flow and structural diagram depicting the
// various elements  (proxy, server  and client)  and how  they
// connect and interact with each other.

//
//                                    ---> upstream --->           +---------------+
//                                                     +---->------>               |
//                               +-----------+         |           | Remote Server |
//                     +--------->          [x]--->----+  +---<---[x]              |
//                     |         | TCP Proxy |            |        +---------------+
// +-----------+       |  +--<--[x] Server   <-----<------+
// |          [x]--->--+  |      +-----------+
// |  Client   |          |
// |           <-----<----+
// +-----------+
//                <--- downstream <---
//
//


#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

std::vector<char> vBuffer(20 * 1024);

namespace tcp_proxy
{
    namespace ip = boost::asio::ip;

    class bridge : public boost::enable_shared_from_this<bridge>
    {
    public:

        typedef ip::tcp::socket socket_type;
        typedef boost::shared_ptr<bridge> ptr_type;

        const std::string LogFileLocation = "log/data.log";

        bridge(boost::asio::io_service& ios)
            : downstream_socket_(ios),
            upstream_socket_(ios)//,
            //temp_socket_(ios)
        {
        }

        void FlushWriteFile(std::string str) {
            //std::stringstream out(LogFileLocation);
            //std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
            //std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!

            //std::cout << "\n" << str << "\n";  //output to the file out.txt

            //std::cout.rdbuf(coutbuf); //reset to standard output again

            std::ofstream myfile;
            myfile.open("example.txt", std::fstream::app);
            myfile << str << "\n";
            myfile.close();
        }

        //void PrintToFS(boost::asio::ip::tcp::socket& socket) {
        //    socket.async_read_some(boost::asio::buffer(vBuffer.data(), vBuffer.size()),
        //        [&](std::error_code ecode, std::size_t len) {

        //            if (!ecode) {
        //                std::cout << "\n\n READ : " << len << " bytes\n\n";

        //                for (int i = 0; i < len; i++) {
        //                    std::cout << vBuffer[i];
        //                }

        //                PrintToFS(socket);
        //            }
        //        }
        //    );
        //}

        //void PrintData(boost::asio::ip::tcp::socket& socket) {
        //    socket.async_read_some(boost::asio::buffer(vBuffer.data(), vBuffer.size()),
        //        [&](std::error_code ecode, std::size_t len) {
        //            
        //            if (!ecode) {
        //                std::cout << "\n\n READ : " << len << " bytes\n\n";

        //                for (int i = 0; i < len; i++) {
        //                    std::cout << vBuffer[i];
        //                }

        //                //PrintData(socket);
        //            }
        //        }
        //    );
        //}

        socket_type& downstream_socket()
        {
            // Client socket
            return downstream_socket_;
        }

        socket_type& upstream_socket()
        {
            // Remote server socket
            return upstream_socket_;
        }

        void start(const std::string& upstream_host, unsigned short upstream_port)
        {
            // Attempt connection to remote server (upstream side)
            upstream_socket_.async_connect(
                ip::tcp::endpoint(
                    boost::asio::ip::address::from_string(upstream_host),
                    upstream_port),
                boost::bind(&bridge::handle_upstream_connect,
                    shared_from_this(),
                    boost::asio::placeholders::error));
        }

        void handle_upstream_connect(const boost::system::error_code& error)
        {
            if (!error)
            {                                
                // Setup async read from remote server (upstream)
                upstream_socket_.async_read_some(
					boost::asio::buffer(upstream_data_, max_data_length),
					boost::bind(&bridge::handle_upstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));

                // Setup async read from client (downstream)
                downstream_socket_.async_read_some(
                    boost::asio::buffer(downstream_data_, max_data_length),
                    boost::bind(&bridge::handle_downstream_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

            }
            else
                close();
        }

    private:

        /*
           Section A: Remote Server --> Proxy --> Client
           Process data recieved from remote sever then send to client.
        */

        // Read from remote server complete, now send data to client
        void handle_upstream_read(const boost::system::error_code& error,
            const size_t& bytes_transferred)
        {

            std::cout << "Bytes transferred to downstream: " << bytes_transferred << "\n";
            std::cout << "Data: " << upstream_data_<< "\n";

            std::string data = "Bytes transferred upstream -> downstream: ";
            data += std::to_string(bytes_transferred);
            data += "\n";

            FlushWriteFile(data);

            data = "Data: ";
            std::string s(reinterpret_cast<char const*>(upstream_data_), bytes_transferred);
            data += s;
            data += "\n";

            FlushWriteFile(data);

            if (!error)
            {
                async_write(downstream_socket_,
					boost::asio::buffer(upstream_data_, bytes_transferred),

					/*[&](std::error_code ecode, std::size_t len) {

                        if (!ecode) {
                            std::cout << "\n\n Read Upstream data: " << len << " bytes\n\n";

                            for (int i = 0; i < len; i++) {
                                std::cout << upstream_data_[i];
                            }*/

                            boost::bind(&bridge::handle_downstream_write,
                                shared_from_this(),
                                boost::asio::placeholders::error)
     //                   }
					//}
				);
			}
            else
                close();
        }

        // Write to client complete, Async read from remote server
        void handle_downstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                //PrintData(upstream_socket_);

                upstream_socket_.async_read_some(
                    boost::asio::buffer(upstream_data_, max_data_length),

                    //[&](std::error_code ecode, std::size_t len) {

                    //    if (!ecode) {
                    //        std::cout << "\n\n Write to downstream : " << len << " bytes\n\n";

                    //        for (int i = 0; i < len; i++) {
                    //            std::cout << upstream_data_[i];
                    //        }

                    //        boost::bind(&bridge::handle_upstream_read,
                    //            shared_from_this(),
                    //            boost::asio::placeholders::error,
                    //            boost::asio::placeholders::bytes_transferred);
                    //    }
                    //});

                    boost::bind(&bridge::handle_upstream_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }
        // *** End Of Section A ***


        /*
           Section B: Client --> Proxy --> Remove Server
           Process data recieved from client then write to remove server.
        */

        // Read from client complete, now send data to remote server
        void handle_downstream_read(const boost::system::error_code& error,
            const size_t& bytes_transferred)
        {
            if (!error)
            {
                std::cout << "Bytes transferred to upstream: " << bytes_transferred << "\n";
                std::cout << "Data: " << downstream_data_ << "\n";

                std::string data = "Bytes transferred downstream -> upstream: ";
                data += std::to_string(bytes_transferred);
                data += "\n";

                FlushWriteFile(data);

                data = "Data: ";
                std::string s(reinterpret_cast<char const*>(downstream_data_), bytes_transferred);
                data += s;
                data += "\n";

                FlushWriteFile(data);

                async_write(upstream_socket_,
                    boost::asio::buffer(downstream_data_, bytes_transferred),

                    /*[&](std::error_code ecode, std::size_t len) {

                        if (!ecode) {
                            std::cout << "\n\n Read Downstream data: " << len << " bytes\n\n";

                            for (int i = 0; i < len; i++) {
                                std::cout << downstream_data_[i];
                            }*/

                            boost::bind(&bridge::handle_upstream_write,
                                shared_from_this(),
                                boost::asio::placeholders::error)
                    //    }
                    //}
                );
            }
            else
                close();
        }

        // Write to remote server complete, Async read from client
        void handle_upstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                //PrintData(downstream_socket_);

                downstream_socket_.async_read_some(
                    boost::asio::buffer(downstream_data_, max_data_length),
                    boost::bind(&bridge::handle_downstream_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else
                close();

        }
        // *** End Of Section B ***

        void close()
        {
            boost::mutex::scoped_lock lock(mutex_);

            if (downstream_socket_.is_open())
            {
                downstream_socket_.close();
            }

            if (upstream_socket_.is_open())
            {
                upstream_socket_.close();
            }
        }

        socket_type downstream_socket_;
        socket_type upstream_socket_;

        //socket_type temp_socket_;

        enum { max_data_length = 8192 }; //8KB
        unsigned char downstream_data_[max_data_length];
        unsigned char upstream_data_[max_data_length];

        //unsigned char temp_data_[max_data_length];

        boost::mutex mutex_;

    public:

        class acceptor
        {
        public:

            acceptor(boost::asio::io_service& io_service,
                const std::string& local_host, unsigned short local_port,
                const std::string& upstream_host, unsigned short upstream_port)
                : io_service_(io_service),
                localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                acceptor_(io_service_, ip::tcp::endpoint(localhost_address, local_port)),
                upstream_port_(upstream_port),
                upstream_host_(upstream_host)
            {}

            bool accept_connections()
            {
                try
                {
                    session_ = boost::shared_ptr<bridge>(new bridge(io_service_));

                    acceptor_.async_accept(session_->downstream_socket(),
                        boost::bind(&acceptor::handle_accept,
                            this,
                            boost::asio::placeholders::error));

                    acceptor_.wait_read();
                }
                catch (std::exception& e)
                {
                    std::cerr << "acceptor exception: " << e.what() << std::endl;
                    return false;
                }

                return true;
            }

        private:

            void handle_accept(const boost::system::error_code& error)
            {
                if (!error)
                {
                    std::cout << "Connection accepted!\n";
                    session_->start(upstream_host_, upstream_port_);

                    if (!accept_connections())
                    {
                        std::cerr << "Failure during call to accept." << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Error: " << error.message() << std::endl;
                }
            }

            boost::asio::io_service& io_service_;
            ip::address_v4 localhost_address;
            ip::tcp::acceptor acceptor_;
            ptr_type session_;
            unsigned short upstream_port_;
            std::string upstream_host_;
        };

    };
}

int main(int argc, char* argv[])
{

    if (argc != 5)
    {
        std::cerr << "usage: tcpproxy_server <local host ip> <local port> <forward host ip> <forward port>" << std::endl;
        return 1;
    }

    const unsigned short local_port = static_cast<unsigned short>(::atoi(argv[2]));
    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[4]));
    const std::string local_host = argv[1];
    const std::string forward_host = argv[3];

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