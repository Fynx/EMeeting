#include <boost/bind.hpp>
#include <iostream>
#include <thread>

#include "Server/EMServer.h"
#include "System/Messages.h"
#include "System/Utils.h"

/**
 * \class EMServer
 */

EMServer::EMServer() :
	AbstractServer(),

	port(EM::Default::PORT),

	fifo_size(EM::Default::FIFO_SIZE),
	fifo_low_watermark(EM::Default::FIFO_LOW_WATERMARK),
	fifo_high_watermark(EM::Default::FIFO_HIGH_WATERMARK),

	buffer_length(EM::Default::BUFFER_LENGTH),

	tx_interval(EM::Default::TX_INTERVAL)
{}

EMServer::~EMServer()
{
	quit();
}

void EMServer::set_port(uint port)
{
	this->port = port;
}

uint EMServer::get_port() const
{
	return port;
}

void EMServer::set_fifo_size(uint fifo_size)
{
	this->fifo_size = fifo_size;
}

uint EMServer::get_fifo_size() const
{
	return fifo_size;
}

void EMServer::set_fifo_low_watermark(uint fifo_low_watermark)
{
	this->fifo_low_watermark = fifo_low_watermark;
}

uint EMServer::get_fifo_low_watermark() const
{
	return fifo_low_watermark;
}

void EMServer::set_fifo_high_watermark(uint fifo_high_watermark)
{
	this->fifo_high_watermark = fifo_high_watermark;
}

uint EMServer::get_fifo_high_watermark() const
{
	return fifo_high_watermark;
}

void EMServer::set_buffer_length(uint buffer_length)
{
	this->buffer_length = buffer_length;
}

uint EMServer::get_buffer_length() const
{
	return buffer_length;
}

void EMServer::set_tx_interval(uint tx_interval)
{
	this->tx_interval = tx_interval;
}

uint EMServer::get_tx_interval() const
{
	return tx_interval;
}

void EMServer::start()
{
	tcp_acceptor = new boost::asio::ip::tcp::acceptor(
		io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	std::cerr << "accepting connections on port " << port << " (IPv4)\n";
	start_accept();

	std::thread (&EMServer::send_info_routine, this).detach();

	io_service.run();
}

void EMServer::quit()
{
	std::cerr << "\nclearing server resources...\n";

	clients[2] = new ClientObject(2, 5, 5, 5);

	std::cerr << "inserted into clients\n";

	std::cerr << "and here it is: " << clients[5]->get_cid() << "\n";

	std::cerr << "deleting\n";

	for (auto obj : clients) {
		std::cerr << "deleting " << obj.second << "\n";
		delete obj.second;
	}
	std::cerr << "done\n";
}

uint EMServer::get_next_cid()
{
	while (true) {
		uint cid = AbstractServer::get_next_cid();
		bool used = false;

		for (auto c : clients)
			if (c.second->get_cid() == cid)
				used = true;
		if (!used)
			return cid;
	}
	std::cerr << "get next cid: error\n";
	exit(EXIT_SUCCESS);
}

void EMServer::add_client(uint cid)
{
	clients[cid] =
		new ClientObject(cid,
		                 get_fifo_size(),
		                 get_fifo_low_watermark(),
		                 get_fifo_high_watermark());
	std::cerr << "added client: " << cid << "\n";
}

void EMServer::on_connection_established(uint cid, Connection *connection)
{
	add_client(cid);
	clients[cid]->set_connection(dynamic_cast<TcpConnection *>(connection));
}

void EMServer::on_connection_lost(uint cid)
{
	clients[cid]->set_connection(nullptr);
}

void EMServer::start_accept()
{
	TcpConnection::Pointer new_connection =
		TcpConnection::create(this, tcp_acceptor->get_io_service());
	std::cerr << "waiting for connections...\n";
	tcp_acceptor->async_accept(
		new_connection->get_socket(),
		boost::bind(&EMServer::handle_accept, this, new_connection,
		            boost::asio::placeholders::error));
}

void EMServer::handle_accept(
	TcpConnection::Pointer new_connection,
        const boost::system::error_code &error)
{
	if (!error)
		new_connection->start();
	else
		std::cerr << "handle_accept: error\n";
	start_accept();
}

void EMServer::send_info_routine()
{
	while (true) {
		sleep(SEND_INFO_FREQUENCY);
		if (get_active_clients_number() > 0) {
			std::cerr << "sending info\n";
			std::string report("\n");
			for (auto p : clients)
				if (p.second->is_connected())
					report += p.second->get_report();
			for (auto p : clients)
				if (p.second->is_connected())
					p.second->get_connection()->send_info(report);
		}
	}
}

uint EMServer::get_active_clients_number() const
{
	uint cnt = 0;
	for (auto p : clients)
		cnt += (int) (p.second->is_connected());
	return cnt;
}
