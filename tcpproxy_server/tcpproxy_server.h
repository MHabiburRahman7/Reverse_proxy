#pragma once

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

namespace tcp_proxy
{
	namespace ip = boost::asio::ip;

	class bridge : public boost::enable_shared_from_this<bridge>
	{

	public:

		typedef ip::tcp::socket socket_type;
		typedef boost::shared_ptr<bridge> ptr_type;

	public:

		class acceptor {

		};
	};
}