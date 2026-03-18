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
	alive_ = true;
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

					// 处理末尾\r
					if (!msg.empty() && msg.back() == '\r')
						msg.pop_back();

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
	if (msg.empty() || msg[0] != '{')
	{
		auto ep = socket_.remote_endpoint();
		std::cout << "Session: Invalid message from " << ep.address().to_string()
			<<": " << msg << std::endl;
		return;
	}

	json j = json::parse(msg, nullptr, false);

	if (j.is_discarded())
	{
		std::cout << "Session: JSON parse failed: " << msg << std::endl;
		return;
	}

	try {
		std::string type = j.at("type");

		if (type == "match") {
			std::string mode = j.at("mode");
			room_manager_->handleMatch(shared_from_this(), mode);
		}
		else if (type == "move")
		{

			if (room_ && player_) {
				std::string from = j.at("from");
				std::string to = j.at("to");
				char promotion = 'q';
				if (j.contains("promotion") && j["promotion"].is_string()) {
					std::string prom = j["promotion"];
					if (!prom.empty()) promotion = prom[0];
				}
				room_->handleMove(player_, from, to, promotion);

			}
		}
		else if (type == "resign")
		{
			if (room_)
				room_->handleResign(player_);
		}
		else if (type == "reconnect")
		{
			auto roomId = j["room_id"];

			room_manager_->handleReconnect(shared_from_this(), roomId);
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Session: JSON parse error: " << e.what() << std::endl;
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

			alive_ = false;
			disconnected_ = true;

			if (room_)
			{
				room_->onPlayerDisconnected(self);
				room_.reset();
			}
			player_.reset();

			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	);
}