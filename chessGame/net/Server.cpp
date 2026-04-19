#include "Server.h"
#include <memory>
#include "Session.h"
#include "RoomManager.h"

Server::Server(boost::asio::io_context& io, short port)
	:acceptor_(io, tcp::endpoint(tcp::v4(), port)),
	room_manager_(std::make_shared<RoomManager>(io))
{
	doAccept();
}

void Server::doAccept() {
	// 收到客户端连接 -> 创建session
	acceptor_.async_accept(
		[this](boost::system::error_code ec, tcp::socket socket) {

			if (!ec) {
				auto session = std::make_shared<Session>(std::move(socket), room_manager_);
				session->start();
			}
			doAccept();
		}
	);
}