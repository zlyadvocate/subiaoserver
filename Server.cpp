#include "Server.h"
#include <iostream>
#include "ConnectionManager.h"
#include "Connection.h"

#define MAX_DATA_SIZE 64*1024

Server::Server(asio::io_service& io_service,
               uint16_t port) :
    acceptor_(io_service),
    running_(false),
    io_service_(io_service),
    m_port(8888)
{
#ifdef TCP_OPTION_NODELAY
    acceptor_.set_option(asio::ip::tcp::no_delay(true));
#endif

    StartAccept();
    running_ = true;
}

Server::~Server()
{
    if (running_)
        Shutdown();
}

void Server::Shutdown()
{
    connectionManager_.StopAll();
    io_service_.stop();
    running_ = false;
}

void Server::StartAccept()
{
    boost::system::error_code ec;
   boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), m_port);
   acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Failed to open acceptor on port " << m_port << " with error: " << ec.message() << std::endl;
       // return false;
    }

    acceptor_.bind(
        boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(),
            m_port
        ),
        ec);

    if (ec)
    {
        std::cerr << "Failed to bind port " << m_port << " with error: " << ec.message() << std::endl;

    }

    acceptor_.listen(socket_base::max_connections, ec);
    if (ec)
    {
        std::cerr << "Failed to listen on port " << m_port << " with error: " << ec.message() << std::endl;

    }
   // printf( "async_accept !\n");
    newConnection_.reset(new Connection(io_service_, connectionManager_));
    acceptor_.async_accept(newConnection_->socket(),
                           std::bind(&Server::HandleAccept, this,
                                     std::placeholders::_1));
}

void Server::HandleAccept(const boost::system::error_code& error)
{
    if (!error)
    {

     //printf( "HandleAccept !\n");
        const auto endp = newConnection_->socket().remote_endpoint();
        //  if (IsIpAllowed(endp))
        {
        std::cerr << "New connection from " <<endp.address() << ":" << endp.port() << std::endl;
             connectionManager_.Start(newConnection_);
        }
        //else
        //  LOG_ERROR << "Connection from " << endp.address() << " not allowed" << std::endl;
    }
     newConnection_.reset(new Connection(io_service_, connectionManager_));
     acceptor_.async_accept(newConnection_->socket(),
                           std::bind(&Server::HandleAccept, this,
                                     std::placeholders::_1));
}

