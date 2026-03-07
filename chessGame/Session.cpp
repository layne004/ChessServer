#include "Session.h"
#include "RoomManager.h"
#include <iostream>
#include <sstream>


Session::Session(tcp::socket socket, std::shared_ptr<RoomManager> roomManager)
	:socket_(std::move(socket)), 
	strand_(boost::asio::make_strand(socket_.get_executor())),
	room_manager_(std::move(roomManager))
{

}

void Session::start() {
	// OPTIMIZE: 客户端可选颜色

	doRead();
}

void Session::send(const std::string& msg)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, msg]()
		{
			bool isWriting = !write_queue_.empty();
			write_queue_.push_back(msg);

			if (!isWriting)
			{
				doWrite();
			}
		}
	);
	
}

void Session::sendJson(const json& j)
{
	send(j.dump() + "\n");
}

void Session::setRoom(std::shared_ptr<GameRoom> room)
{
	room_ = room;
}

std::shared_ptr<GameRoom> Session::getRoom() const
{
	return room_;
}

void Session::clearRoom()
{
	room_.reset();
}

void Session::setPlayer(std::shared_ptr<Player> p)
{
	player_ = p;
}

std::shared_ptr<Player> Session::getPlayer()
{
	return player_;
}

void Session::doWrite()
{
	auto self = shared_from_this();
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(write_queue_.front()),
		boost::asio::bind_executor(
			strand_,
			[this, self](boost::system::error_code ec, std::size_t) {
				if (!ec) {
					write_queue_.pop_front();
					if (!write_queue_.empty())
					{
						doWrite();
					}
					
				}else {
					disconnect();
				}

			})
	);
}

void Session::doRead() {
	auto self(shared_from_this());
	boost::asio::async_read_until(
		socket_, buffer_, '\n',
		boost::asio::bind_executor(
			strand_,
			[this, self](boost::system::error_code ec, std::size_t) {
				if (!ec) {
					std::string msg;
					std::istream is(&buffer_);
					std::getline(is, msg);

					handleMessage(msg);
					doRead();
				}
				else {
					disconnect();
				}
			}
		)
	);
}

//void Session::handleMessage(const std::string& msg)
//{
//	// 例：MOVE e2 e4
//	std::istringstream iss(msg);
//	std::string cmd, from, to;
//	iss >> cmd >> from >> to;
//
//	if (cmd == "MOVE") {
//		Color color = room_->getPlayerColor(shared_from_this());
//		room_->handleMove(shared_from_this(), color, from, to);
//		
//	}
//}

void Session::handleMessage(const std::string& msg)
{
	try {
		json j = json::parse(msg);
		std::string type = j["type"];

		if (type == "match") {
			std::string mode = j.at("mode");
			room_manager_->handleMatch(shared_from_this(), mode);
		}
		else if (type == "move")
		{

			if (room_ && player_) {
				auto data = j["data"];
				std::string from = data["from"];
				std::string to = data["to"];

				room_->handleMove(player_, from, to);

			}
		}
		else if (type == "resign")
		{
			if (room_)
				room_->handleResign(player_);
		}
	}
	catch (std::exception& e)
	{
		std::cout << "JSON parse error: " << e.what() << std::endl;
		json err = {
			{"type", "error"},
			{"message", e.what()}
		};
		sendJson(err);
	}

}

// disconnect也添加进post，防止重复断开
void Session::disconnect() {
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self]()
		{
			if (disconnected_)
				return;

			disconnected_ = true;

			if (room_)
			{
				room_manager_->leaveRoom(self);
				room_.reset();
			}

			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	);
}