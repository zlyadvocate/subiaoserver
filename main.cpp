#include <iostream>
#include <memory>
#include "Server.h"
using namespace std;

int main()
{
    cout << "808 active safety sensor server started!\n" << endl;
    asio::io_service ioService_;
    Server server_(ioService_,  '8888');
   // server_ = std::make_unique<Server>(ioService_, "127.0.0.1", "8888");
   ioService_.run();
    return 0;
}
