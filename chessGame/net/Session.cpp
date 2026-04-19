#include "Session.h"
#include "LessonController.h"
#include "RoomManager.h"
#include "Player.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

Session::Session(tcp::socket socket, std::shared_ptr<RoomManager> roomManager)
	: socket_(std::move(socket)),
	strand_(boost::asio::make_strand(socket_.get_executor())),
	buffer_(kMaxMessageBytes + 1),
	room_manager_(std::move(roomManager))
{
}

void Session::start()
{
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
			const bool isWriting = !write_queue_.empty();
			write_queue_.push_back(msg);

			if (!isWriting)
			{
				doWrite();
			}
		});
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

void Session::sendConnectionMessage()
{
	if (!socket_.is_open()) {
		std::cerr << "Socket is not open. Cannot send message." << std::endl;
		return;
	}

	json response;
	response["type"] = "connected";
	response["status"] = "success";
	response["message"] = "connection success!";
	sendJson(response);
}

void Session::doWrite()
{
	auto self = shared_from_this();
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(write_queue_.front()),
		boost::asio::bind_executor(
			strand_,
			[this, self](boost::system::error_code ec, std::size_t)
			{
				if (!ec) {
					write_queue_.pop_front();
					if (!write_queue_.empty())
					{
						doWrite();
					}
				}
				else {
					disconnect();
				}
			}));
}

void Session::doRead()
{
	auto self = shared_from_this();
	boost::asio::async_read_until(
		socket_, buffer_, '\n',
		boost::asio::bind_executor(
			strand_,
			[this, self](boost::system::error_code ec, std::size_t)
			{
				if (!ec) {
					std::string msg;
					std::istream is(&buffer_);
					std::getline(is, msg);

					if (!msg.empty() && msg.back() == '\r') {
						msg.pop_back();
					}

					if (msg.size() > kMaxMessageBytes) {
						sendProtocolError("MESSAGE_TOO_LARGE", "message exceeds max frame size");
						disconnect();
						return;
					}

					handleMessage(msg);
					doRead();
				}
				else {
					disconnect();
				}
			}));
}

void Session::handleMessage(const std::string& msg)
{
	if (msg.empty() || msg[0] != '{')
	{
		auto ep = socket_.remote_endpoint();
		std::cout << "Session: Invalid message from " << ep.address().to_string()
			<< ": " << msg << std::endl;
		sendProtocolError("INVALID_FRAME", "message must be single-line json object");
		return;
	}

	json j = json::parse(msg, nullptr, false);

	if (j.is_discarded())
	{
		std::cout << "Session: JSON parse failed: " << msg << std::endl;
		sendProtocolError("INVALID_JSON", "json parse failed");
		return;
	}

	if (!j.is_object()) {
		sendProtocolError("INVALID_JSON_TYPE", "json payload must be an object", &j);
		return;
	}

	if (!j.contains("type") || !j["type"].is_string()) {
		sendProtocolError("MISSING_TYPE", "field 'type' is required and must be string", &j);
		return;
	}

	try {
		dispatchMessage(j);
	}
	catch (std::exception& e)
	{
		std::cout << "Session: JSON parse error: " << e.what() << std::endl;
		sendProtocolError("BAD_REQUEST", e.what(), &j);
	}
}

void Session::dispatchMessage(const json& j)
{
	static const std::unordered_map<std::string, void (Session::*)(const json&)> handlers = {
		{"match", &Session::handleMatchMessage},
		{"create_room", &Session::handleCreateRoomMessage},
		{"join_room", &Session::handleJoinRoomMessage},
		{"start_lesson", &Session::handleStartLessonMessage},
		{"next_lesson", &Session::handleNextLessonMessage},
		{"cancel_match", &Session::handleCancelMatchMessage},
		{"close_room", &Session::handleCloseRoomMessage},
		{"move", &Session::handleMoveMessage},
		{"lesson_move", &Session::handleLessonMoveMessage},
		{"resign", &Session::handleResignMessage},
		{"reconnect", &Session::handleReconnectMessage},
	};

	const std::string type = j.at("type").get<std::string>();
	const auto it = handlers.find(type);
	if (it == handlers.end()) {
		sendProtocolError("UNKNOWN_TYPE", "unsupported message type: " + type, &j);
		return;
	}

	(this->*(it->second))(j);
}

void Session::handleMatchMessage(const json& j)
{
	std::string mode;
	if (!tryGetStringField(j, "mode", mode, "INVALID_MATCH_MODE",
		"field 'mode' is required and must be string")) {
		return;
	}

	std::string level = "easy";
	std::string color = "random";

	if (j.contains("level") && j["level"].is_string()) {
		level = j["level"].get<std::string>();
	}

	if (j.contains("color") && j["color"].is_string()) {
		color = j["color"].get<std::string>();
	}

	const int initial = getClampedIntField(j, "initial", 300000, 10000, 7200000);
	const int increment = getClampedIntField(j, "increment", 3000, 0, 60000);

	room_manager_->handleMatch(shared_from_this(), mode, level, color, initial, increment);
}

void Session::handleCreateRoomMessage(const json& j)
{
	const int initial = getClampedIntField(j, "initial", 300000, 10000, 7200000, true);
	const int increment = getClampedIntField(j, "increment", 3000, 0, 60000, true);
	room_manager_->createRoom(shared_from_this(), initial, increment);
}

void Session::handleJoinRoomMessage(const json& j)
{
	std::string roomCode;
	if (!tryGetStringField(j, "room_code", roomCode, "INVALID_JOIN_ROOM_CODE",
		"field 'room_code' is required and must be string")) {
		return;
	}

	room_manager_->joinRoom(shared_from_this(), roomCode);
}

void Session::handleStartLessonMessage(const json& j)
{
	std::string lessonId;
	if (!tryGetStringField(j, "lesson_id", lessonId, "INVALID_LESSON_ID",
		"field 'lesson_id' is required and must be string")) {
		return;
	}

	room_manager_->createLessonRoom(shared_from_this(), lessonId);
}

void Session::handleNextLessonMessage(const json& j)
{
	std::string currentLessonId;
	if (!tryGetStringField(j, "current_lesson_id", currentLessonId, "INVALID_LESSON_ID",
		"field 'current_lesson_id' is required and must be string")) {
		return;
	}

	const auto nextLessonId = LessonController::findNextLessonId(currentLessonId);
	if (!nextLessonId.has_value()) {
		sendProtocolError("LESSON_NOT_FOUND", "next lesson not found", &j);
		return;
	}

	room_manager_->createLessonRoom(shared_from_this(), *nextLessonId);
}

void Session::handleCancelMatchMessage(const json&)
{
	room_manager_->cancelMatch(shared_from_this());
}

void Session::handleCloseRoomMessage(const json&)
{
	room_manager_->closeRoom(shared_from_this());
}

void Session::handleMoveMessage(const json& j)
{
	if (!room_ || !player_) {
		return;
	}

	std::string from;
	std::string to;
	char promotion = 'q';
	if (!tryGetMovePayload(j, from, to, promotion)) {
		return;
	}

	room_->handleMove(player_, from, to, promotion);
}

void Session::handleLessonMoveMessage(const json& j)
{
	if (!room_ || !player_) {
		return;
	}

	std::string from;
	std::string to;
	char promotion = 'q';
	if (!tryGetMovePayload(j, from, to, promotion)) {
		return;
	}

	room_->handleLessonMove(player_, from, to, promotion);
}

void Session::handleResignMessage(const json&)
{
	if (room_) {
		room_->handleResign(player_);
	}
}

void Session::handleReconnectMessage(const json& j)
{
	if (!j.contains("room_id") || !j.contains("player_id") || !j["player_id"].is_string()) {
		sendProtocolError("INVALID_RECONNECT_FIELDS", "fields 'room_id' and 'player_id' are required", &j);
		return;
	}

	const auto roomId = j.at("room_id").get<GameRoom::RoomID>();
	const auto playerId = j.at("player_id").get<std::string>();
	room_manager_->handleReconnect(shared_from_this(), roomId, playerId);
}

void Session::sendProtocolError(const std::string& code, const std::string& message, const json* request)
{
	json err = {
		{"type", "error"},
		{"code", code},
		{"message", message}
	};
	if (request != nullptr && request->is_object() && request->contains("request_id")) {
		err["request_id"] = (*request)["request_id"];
	}
	sendJson(err);
}

bool Session::tryGetStringField(const json& j, const char* field, std::string& out,
	const char* errorCode, const char* errorMessage)
{
	if (!j.contains(field) || !j[field].is_string()) {
		sendProtocolError(errorCode, errorMessage, &j);
		return false;
	}

	out = j.at(field).get<std::string>();
	return true;
}

int Session::getClampedIntField(const json& j, const char* field, int defaultValue,
	int minValue, int maxValue, bool integerOnly) const
{
	if (!j.contains(field)) {
		return defaultValue;
	}

	const bool validNumber = integerOnly ? j[field].is_number_integer() : j[field].is_number();
	if (!validNumber) {
		return defaultValue;
	}

	return std::clamp(j[field].get<int>(), minValue, maxValue);
}

bool Session::tryGetMovePayload(const json& j, std::string& from, std::string& to, char& promotion)
{
	if (!j.contains("from") || !j["from"].is_string() || !j.contains("to") || !j["to"].is_string()) {
		sendProtocolError("INVALID_MOVE_FIELDS", "fields 'from' and 'to' are required and must be strings", &j);
		return false;
	}

	from = j.at("from").get<std::string>();
	to = j.at("to").get<std::string>();

	if (!isValidSquare(from) || !isValidSquare(to)) {
		sendProtocolError("INVALID_MOVE_SQUARE", "move square must be algebraic format like e2/e4", &j);
		return false;
	}

	promotion = 'q';
	if (j.contains("promotion") && j["promotion"].is_string()) {
		const std::string prom = j["promotion"].get<std::string>();
		if (!prom.empty()) {
			promotion = prom[0];
		}
	}

	return true;
}

bool Session::isValidSquare(const std::string& square)
{
	if (square.size() != 2) {
		return false;
	}
	return (square[0] >= 'a' && square[0] <= 'h') && (square[1] >= '1' && square[1] <= '8');
}

void Session::disconnect()
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self]()
		{
			if (disconnected_)
				return;

			alive_ = false;
			disconnected_ = true;

			if (room_manager_) {
				room_manager_->handleSessionClosed(self);
			}

			if (room_)
			{
				const std::string playerId = player_ ? player_->id() : "";
				room_->onPlayerDisconnected(self, playerId);
				room_.reset();
			}
			player_.reset();

			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		});
}
