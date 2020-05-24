#include "ConnectionManager.h"

//E:\comms\server\ABx-master\abdata\abdata

void ConnectionManager::Start(std::shared_ptr<Connection> c)
{
    printf("ConnectionManager::Start\n");
    connections_.insert(c);
    c->Start();
}

void ConnectionManager::Stop(std::shared_ptr<Connection> c)
{
    printf("ConnectionManager::Stop\n");
    connections_.erase(c);
    c->Stop();
}

void ConnectionManager::StopAll()
{
    std::for_each(connections_.begin(), connections_.end(),
                  std::bind(&Connection::Stop, std::placeholders::_1));
    connections_.clear();
}

void ConnectionManager::BroadcastAll()
{
    //for (auto rit=connections_.begin(); rit != connections_.end(); ++rit)
    //std::cout << ' ' << *rit;
}
