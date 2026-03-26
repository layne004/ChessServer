#pragma once
// 只负责接人，不负责业务
#include <boost/asio.hpp>
#include <memory>
using namespace boost::asio::ip;

class RoomManager;

class Server
{
public:
	Server(boost::asio::io_context& io, short port);

private:
	void doAccept();

	tcp::acceptor acceptor_;
	std::shared_ptr<RoomManager> room_manager_;
};

