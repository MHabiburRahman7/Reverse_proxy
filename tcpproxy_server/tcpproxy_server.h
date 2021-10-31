#pragma once

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/thread.hpp>
#include "io_service_pool.h"

// thread pooling
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
		{
			myfile.open(LogFileLocation, std::fstream::app);
		}

		~bridge() {
			myfile.close();
		}

		void FlushWriteFile(std::string str);
		socket_type& downstream_socket();
		socket_type& upstream_socket();

		void start(const std::string& upstream_host, unsigned short upstream_port);

		//std::thread runConnectThread(const std::string& upstream_host, unsigned short upstream_port);
		//void run(const std::string& upstream_host, unsigned short upstream_port);
		void handle_upstream_connect(const boost::system::error_code& error);

	private:

		socket_type downstream_socket_;
		socket_type upstream_socket_;

		enum { max_data_length = 8192 }; //8KB
		unsigned char downstream_data_[max_data_length];
		unsigned char upstream_data_[max_data_length];

		//file handler
		std::ofstream myfile;
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
			acceptor(const std::string& local_host, unsigned short local_port,
				const std::string& upstream_host, unsigned short upstream_port,
				std::size_t io_service_pool_size)
				: localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
				acceptor_(io_service_pool_.get_io_service(), ip::tcp::endpoint(localhost_address, local_port)),
				upstream_port_(upstream_port),
				upstream_host_(upstream_host),
				io_service_pool_(io_service_pool_size)
			{
				//instantiate the io_service pool based on available threads
			}

			bool accept_connections();

			void init(const std::string& local_host, unsigned short local_port,
				const std::string& upstream_host, unsigned short upstream_port,
				std::size_t io_service_pool_size);
			void runAcceptor();

		private:

			/// The pool of io_service objects used to perform asynchronous operations.
			io_service_pool io_service_pool_;

			ip::address_v4 localhost_address;
			ip::tcp::acceptor acceptor_;

			boost::shared_ptr<bridge> session_;
			unsigned short upstream_port_;
			std::string upstream_host_;

			void handle_accept(const boost::system::error_code& error);
		};

	};
};