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
	static constexpr std::size_t kMaxMessageBytes = 64 * 1024; // 64KB single-line JSON frame

	Session(tcp::socket socket, std::shared_ptr<RoomManager> roomManager);
	void start();

	void send(const std::string& msg);
	void sendJson(const json& j);

	void setRoom(std::shared_ptr<GameRoom> room);
	std::shared_ptr<GameRoom> getRoom() const;
	void clearRoom();

	void setPlayer(std::shared_ptr<Player> p);
	std::shared_ptr<Player> getPlayer();

	bool isAlive() { return alive_; }
	void sendConnectionMessage();

private:
	void doRead();
	void doWrite();
	void handleMessage(const std::string& msg);
	void dispatchMessage(const json& j);
	void handleMatchMessage(const json& j);
	void handleCreateRoomMessage(const json& j);
	void handleJoinRoomMessage(const json& j);
	void handleCancelMatchMessage(const json& j);
	void handleCloseRoomMessage(const json& j);
	void handleMoveMessage(const json& j);
	void handleLessonMoveMessage(const json& j);
	void handleResignMessage(const json& j);
	void handleReconnectMessage(const json& j);
	void disconnect();
	void sendProtocolError(const std::string& code, const std::string& message, const json* request = nullptr);
	bool tryGetStringField(const json& j, const char* field, std::string& out, const char* errorCode, const char* errorMessage);
	int getClampedIntField(const json& j, const char* field, int defaultValue, int minValue, int maxValue, bool integerOnly = false) const;
	bool tryGetMovePayload(const json& j, std::string& from, std::string& to, char& promotion);
	static bool isValidSquare(const std::string& square);

	std::atomic<bool> alive_{true};
	tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	boost::asio::streambuf buffer_;
	std::deque<std::string> write_queue_;

	std::shared_ptr<Player> player_;
	std::shared_ptr<GameRoom> room_;
	std::shared_ptr<RoomManager> room_manager_;
	bool disconnected_ = false;
};

