#include "Session.h"
#include "RoomManager.h"
#include <iostream>
#include <sstream>
#include "Player.h"

Session::Session(tcp::socket socket, std::shared_ptr<RoomManager> roomManager)
	:socket_(std::move(socket)), 
	strand_(boost::asio::make_strand(socket_.get_executor())),
	room_manager_(std::move(roomManager))
{

}

void Session::start() {
	
	alive_ = true;

	sendConnectionMessage();

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
	if (j.is_null()) {
		std::cerr << "[sendJson] Warning: sending null JSON" << std::endl;
	}
	else {
		std::cout << "[sendJson] Sending JSON: " << j.dump() << std::endl;
	}
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

void Session::sendConnectionMessage() {
	auto self = shared_from_this();

	if (socket_.is_open()) {
		json response;
		response["type"] = "connected";
		response["status"] = "success";
		response["message"] = "connection success!";
		sendJson(response);
	}
	else {
		std::cerr << "Socket is not open. Cannot send message." << std::endl;
	}
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

			std::string level = "easy";
			std::string color = "random";

			if (j.contains("level"))
				level = j["level"].get<std::string>();

			if (j.contains("color"))
				color = j["color"].get<std::string>();

			// 新增：时间控制解析，默认5+3
			int initial = 300000;
			int increment = 3000; 

			if (j.contains("initial") && j["initial"].is_number())
				initial = j["initial"].get<int>();

			if (j.contains("increment") && j["increment"].is_number())
				increment = j["increment"].get<int>();

			room_manager_->handleMatch(shared_from_this(), mode, level, color, initial, increment);
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
			auto roomId = j.at("room_id").get<GameRoom::RoomID>();
			auto playerId = j.at("player_id").get<std::string>();

			room_manager_->handleReconnect(shared_from_this(), roomId, playerId);
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
				std::string playerId = player_ ? player_->id() : "";
				room_->onPlayerDisconnected(self, playerId);
				room_.reset();
			}
			player_.reset();

			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	);
}