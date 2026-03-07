#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <string>
#include "GameRoom.h"
#include <deque>
using namespace boost::asio::ip;
#include <json.hpp>
using json = nlohmann::json;

class RoomManager;

class Player;

class Session:public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket socket, std::shared_ptr<RoomManager> roomManager);
	void start();

	void send(const std::string& msg);
	void sendJson(const json& j);

	void setRoom(std::shared_ptr<GameRoom> room);
	std::shared_ptr<GameRoom> getRoom() const;
	void clearRoom();

	void setPlayer(std::shared_ptr<Player> p);
	std::shared_ptr<Player> getPlayer();

private:
	void doRead();
	void doWrite();
	void handleMessage(const std::string& msg);
	void disconnect();

	tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	boost::asio::streambuf buffer_;
	std::deque<std::string> write_queue_;

	std::shared_ptr<Player> player_;
	std::shared_ptr<GameRoom> room_;
	std::shared_ptr<RoomManager> room_manager_;
	bool disconnected_ = false;
};

