#pragma once

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

namespace tcp_proxy
{
	namespace ip = boost::asio::ip;

	class bridge : public boost::enable_shared_from_this<bridge>
	{

	public:

		typedef ip::tcp::socket socket_type;
		typedef boost::shared_ptr<bridge> ptr_type;

		const std::string LogFileLocation = "dataDump.txt";

		bridge(boost::asio::io_service& ios)
			: downstream_socket_(ios),
			upstream_socket_(ios)
		{}

		void FlushWriteFile(std::string str);
		socket_type& downstream_socket();
		socket_type& upstream_socket();

		void start(const std::string& upstream_host, unsigned short upstream_port);
		void handle_upstream_connect(const boost::system::error_code& error);

	private:

		socket_type downstream_socket_;
		socket_type upstream_socket_;

		enum { max_data_length = 8192 }; //8KB
		unsigned char downstream_data_[max_data_length];
		unsigned char upstream_data_[max_data_length];

		boost::mutex mutex_;

		void handle_upstream_read(const boost::system::error_code& error,
			const size_t& bytes_transferred);
		void handle_downstream_write(const boost::system::error_code& error);
		void handle_downstream_read(const boost::system::error_code& error,
			const size_t& bytes_transferred);
		void handle_upstream_write(const boost::system::error_code& error);
		void close();

	public:

		class acceptor {

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

			bool accept_connections();

		private:

			boost::asio::io_service& io_service_;
			ip::address_v4 localhost_address;
			ip::tcp::acceptor acceptor_;
			ptr_type session_;
			unsigned short upstream_port_;
			std::string upstream_host_;

			void handle_accept(const boost::system::error_code& error);

		};
	};
}