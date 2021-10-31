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

#include "tcpproxy_server.h"

using namespace tcp_proxy;
using namespace ip;

void tcp_proxy::bridge::FlushWriteFile(std::string str)
{
	//lock to avoid racing condition
	mutex_.lock();

	myfile << str << "\n";
	myfile.flush();

	//unlock to release the thread
	mutex_.unlock();
}

bridge::socket_type& tcp_proxy::bridge::downstream_socket()
{
	// Client socket
	return downstream_socket_;
}

bridge::socket_type& tcp_proxy::bridge::upstream_socket()
{
	// Remote server socket
	return upstream_socket_;
}

void tcp_proxy::bridge::start(const std::string& upstream_host, unsigned short upstream_port)
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

void tcp_proxy::bridge::handle_upstream_connect(const boost::system::error_code& error)
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

/*
   Section A: Remote Server --> Proxy --> Client
   Process data recieved from remote sever then send to client.
*/

// Read from remote server complete, now send data to client
void tcp_proxy::bridge::handle_upstream_read(const boost::system::error_code& error,
	const size_t& bytes_transferred)
{
	//Dump raw data to the file
	std::cout << "Bytes transferred to downstream: " << bytes_transferred << "\n";
	std::cout << "Data: " << upstream_data_ << "\n";

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
		//lock to avoid racing condition
		mutex_.lock();

		async_write(downstream_socket_,
			boost::asio::buffer(upstream_data_, bytes_transferred),
			boost::bind(&bridge::handle_downstream_write,
				shared_from_this(),
				boost::asio::placeholders::error));

		mutex_.unlock();
	}
	else
		close();
}

// Write to client complete, Async read from remote server
void tcp_proxy::bridge::handle_downstream_write(const boost::system::error_code& error)
{
	if (!error)
	{
		upstream_socket_.async_read_some(
			boost::asio::buffer(upstream_data_, max_data_length),
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
void tcp_proxy::bridge::handle_downstream_read(const boost::system::error_code& error,
	const size_t& bytes_transferred)
{
	//Dump raw data to the file
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

	if (!error)
	{
		//lock to avoid racing condition
		mutex_.lock();

		async_write(upstream_socket_,
			boost::asio::buffer(downstream_data_, bytes_transferred),
			boost::bind(&bridge::handle_upstream_write,
				shared_from_this(),
				boost::asio::placeholders::error));

		mutex_.unlock();
	}
	else
		close();
}

// Write to remote server complete, Async read from client
void tcp_proxy::bridge::handle_upstream_write(const boost::system::error_code& error)
{
	if (!error)
	{
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

void tcp_proxy::bridge::close()
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

bool tcp_proxy::bridge::acceptor::accept_connections()
{
	try
	{
		session_ = boost::shared_ptr<bridge>(new bridge(io_service_pool_.get_io_service()));

		//1 acceptor = 1 port
		//But we have several io_service on n threads which will handle the new concurrent connection
		acceptor_.async_accept(session_->downstream_socket(),
			boost::bind(&acceptor::handle_accept,
				this,
				boost::asio::placeholders::error));
	}
	catch (std::exception& e)
	{
		std::cerr << "acceptor exception: " << e.what() << std::endl;
		return false;
	}

	return true;
}


void tcp_proxy::bridge::acceptor::handle_accept(const boost::system::error_code& error)
{
	if (!error)
	{
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

void tcp_proxy::bridge::acceptor::init(const std::string& local_host, unsigned short local_port,
	const std::string& upstream_host, unsigned short upstream_port,
	std::size_t io_service_pool_size)
{
}

void tcp_proxy::bridge::acceptor::runAcceptor()
{
	io_service_pool_.run();
}
