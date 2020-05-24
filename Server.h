#pragma once

#include "Connection.h"
#include "ConnectionManager.h"

using namespace boost;
class Server {

public:
	Server(asio::io_service& io_service,
        uint16_t port
        );
    ~Server();
    void Shutdown();


private:
	void StartAccept();
	void HandleAccept(const boost::system::error_code& error);
   // bool IsIpAllowed(const asio::ip::tcp::endpoint& ep);
    bool running_;
	asio::io_service& io_service_;
	asio::ip::tcp::acceptor acceptor_;
	std::uint16_t m_port;
    std::shared_ptr<Connection> newConnection_;
	ConnectionManager connectionManager_;
	//StorageProvider storageProvider_;
  //  Net::IpList& whiteList_;

};
