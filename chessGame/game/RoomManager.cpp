#include "RoomManager.h"
#include "Session.h"
#include "NetworkPlayer.h"
#include "AIPlayer.h"
#include "LessonController.h"
#include <random>
#include <cctype>

RoomManager::RoomManager(boost::asio::io_context& io)
	:io_(io)
{
}

void RoomManager::cleanupRooms()
{
	std::lock_guard<std::mutex> lock(mutex_);
	purgeExpiredRoomCodeCacheLocked();
	purgeExpiredWaitingRoomsLocked();

	for (auto it = rooms_.begin(); it != rooms_.end();)
	{
		const bool isWaitingCreatedRoom = roomHostPlayersById_.find(it->first) != roomHostPlayersById_.end();
		if (it->second->isEmpty() && !isWaitingCreatedRoom) {
			eraseRoomMappingsLocked(it->first);
			waitingRoomCreatedAtById_.erase(it->first);
			it = rooms_.erase(it);
		}
		else {
			++it;
		}
	}
}

void RoomManager::handleReconnect(std::shared_ptr<Session> session, GameRoom::RoomID roomId, const std::string& playerId)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (session->getRoom())
	{
		session->sendJson(
			{
				{"type", "error"},
				{"code", "ALREADY_IN_ROOM"},
				{"message", "reconnect failed: already in room"}
			}
		);
		return;
	}

	auto it = rooms_.find(roomId);

	if (it == rooms_.end())
	{
		session->sendJson(
			{
				{"type", "error"},
				{"code", "ROOM_NOT_FOUND"},
				{"message", "reconnect failed: room not found"}
			}
		);
		return;
	}

	auto room = it->second;

	room->reconnect(session, playerId);
}

void RoomManager::handleMatch(std::shared_ptr<Session> session, const std::string& mode
	, const std::string& level, const std::string& color, int initial, int increment)
{
	cleanupRooms();

	if (mode == "pvp") {
		matchPvp(session, initial, increment);
	}
	else if (mode == "pve")
	{
		createPveRoom(session, level, color);
	}
}

void RoomManager::matchPvp(std::shared_ptr<Session> session, int initial, int increment)
{
	std::lock_guard<std::mutex> lock(mutex_);

	// 如果已经在匹配队列就拒绝
	if (session->getRoom())
		return;

	removeFromWaitingBucketsLocked(session);

	std::string key = makeBucketKey(initial, increment);
	auto& queue = waitingBuckets_[key];

	// 清理失效session
	while (!queue.empty())
	{
		auto s = queue.front();

		if (!s || !s->isAlive())
			queue.pop();
		else
			break;
	}

	if (queue.empty()) {
		queue.push(session);

		session->sendJson({
			{"type", "matching"},
			{"mode", "pvp"},
			{"initial", initial},
			{"increment", increment}
		});
	}
	else {
		auto opponent = queue.front();
		queue.pop();

		auto roomId = nextRoomId_++;
		auto room = std::make_shared<GameRoom>(io_, roomId, GameRoom::Mode::PvP, initial, increment);
		rooms_[roomId] = room;

		session->setRoom(room);
		opponent->setRoom(room);

		auto white = std::make_shared<NetworkPlayer>(opponent, Color::White);
		auto black = std::make_shared<NetworkPlayer>(session, Color::Black);

		opponent->setPlayer(white);
		session->setPlayer(black);

		room->start(white, black);
	}
}

void RoomManager::createPveRoom(std::shared_ptr<Session> session, const std::string& level, const std::string& color)
{
	std::lock_guard<std::mutex> lock(mutex_);

	Color humanColor = Color::White;
	if (color == "black")
		humanColor = Color::Black;
	else if (color == "random")
		humanColor = (rand() % 2) ? Color::White : Color::Black;

	Color aiColor = (humanColor == Color::White) ? Color::Black : Color::White;

	//时间
	int initial = 300000;
	int incre = 3000;

	auto roomId = nextRoomId_++;

	auto room = std::make_shared<GameRoom>(io_, roomId, GameRoom::Mode::PvE, initial, incre);
	rooms_[roomId] = room;

	auto human = std::make_shared<NetworkPlayer>(session, humanColor);
	auto ai = std::make_shared<AIPlayer>(aiColor, level);

	session->setRoom(room);
	session->setPlayer(human);

	if (humanColor == Color::White)
		room->start(human, ai);
	else
		room->start(ai, human);
}

void RoomManager::createRoom(std::shared_ptr<Session> session, int initial, int increment)
{
	cleanupRooms();
	std::lock_guard<std::mutex> lock(mutex_);
	purgeExpiredRoomCodeCacheLocked();
	purgeExpiredWaitingRoomsLocked();

	if (session->getRoom()) {
		session->sendJson({
			{"type", "error"},
			{"code", "ALREADY_IN_ROOM"},
			{"message", "create room failed: already in room"}
		});
		return;
	}

	removeFromWaitingBucketsLocked(session);

	auto roomId = nextRoomId_++;
	auto room = std::make_shared<GameRoom>(io_, roomId, GameRoom::Mode::PvP, initial, increment);
	auto roomCode = generateRoomCodeLocked();

	rooms_[roomId] = room;
	roomCodesById_[roomId] = roomCode;
	roomIdByCode_[roomCode] = roomId;
	waitingRoomCreatedAtById_[roomId] = SteadyClock::now();

	session->setRoom(room);
	auto host = std::make_shared<NetworkPlayer>(session, Color::White);
	session->setPlayer(host);
	roomHostPlayersById_[roomId] = host;

	session->sendJson({
		{"type", "room_created"},
		{"room_id", roomId},
		{"room_code", roomCode},
		{"player_id", host->id()},
		{"color", "white"},
		{"initial", initial},
		{"increment", increment}
	});

	session->sendJson({
		{"type", "room_waiting"},
		{"room_id", roomId},
		{"room_code", roomCode},
		{"message", "waiting for opponent"}
	});
}

void RoomManager::joinRoom(std::shared_ptr<Session> session, const std::string& roomCode)
{
	cleanupRooms();
	std::lock_guard<std::mutex> lock(mutex_);
	purgeExpiredRoomCodeCacheLocked();
	purgeExpiredWaitingRoomsLocked();
	std::string normalizedCode = roomCode;
	for (char& c : normalizedCode) {
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}

	if (session->getRoom()) {
		session->sendJson({
			{"type", "error"},
			{"code", "ALREADY_IN_ROOM"},
			{"message", "join room failed: already in room"}
		});
		return;
	}

	removeFromWaitingBucketsLocked(session);

	auto roomIdIt = roomIdByCode_.find(normalizedCode);
	if (roomIdIt == roomIdByCode_.end()) {
		if (expiredRoomCodes_.find(normalizedCode) != expiredRoomCodes_.end()) {
			session->sendJson({
				{"type", "error"},
				{"code", "ROOM_EXPIRED"},
				{"message", "join room failed: room expired"}
			});
			return;
		}
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_NOT_FOUND"},
			{"message", "join room failed: room not found"}
		});
		return;
	}

	auto roomIt = rooms_.find(roomIdIt->second);
	if (roomIt == rooms_.end()) {
		roomIdByCode_.erase(roomIdIt);
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_NOT_FOUND"},
			{"message", "join room failed: room not found"}
		});
		return;
	}

	auto room = roomIt->second;
	auto createdAtIt = waitingRoomCreatedAtById_.find(room->id());
	if (createdAtIt != waitingRoomCreatedAtById_.end()) {
		const auto now = SteadyClock::now();
		if (now - createdAtIt->second > kWaitingRoomTimeout) {
			eraseRoomMappingsLocked(room->id());
			waitingRoomCreatedAtById_.erase(room->id());
			rooms_.erase(room->id());
			session->sendJson({
				{"type", "error"},
				{"code", "ROOM_EXPIRED"},
				{"message", "join room failed: room expired"}
			});
			return;
		}
	}

	if (room->isFull()) {
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_FULL"},
			{"message", "join room failed: room is full"}
		});
		return;
	}

	auto hostIt = roomHostPlayersById_.find(room->id());
	if (hostIt == roomHostPlayersById_.end() || !hostIt->second) {
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_HOST_OFFLINE"},
			{"message", "join room failed: room host unavailable"}
		});
		return;
	}

	auto host = hostIt->second;
	auto hostNet = std::dynamic_pointer_cast<NetworkPlayer>(host);
	if (!hostNet || !hostNet->getSession() || !hostNet->getSession()->isAlive()) {
		eraseRoomMappingsLocked(room->id());
		waitingRoomCreatedAtById_.erase(room->id());
		rooms_.erase(room->id());
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_HOST_OFFLINE"},
			{"message", "join room failed: room host disconnected"}
		});
		return;
	}

	eraseRoomMappingsLocked(room->id());
	waitingRoomCreatedAtById_.erase(room->id());

	auto guest = std::make_shared<NetworkPlayer>(session, Color::Black);
	session->setRoom(room);
	session->setPlayer(guest);

	room->start(host, guest);
}

void RoomManager::cancelMatch(std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (removeFromWaitingBucketsLocked(session)) {
		session->sendJson({
			{"type", "match_cancelled"}
		});
		return;
	}

	session->sendJson({
		{"type", "error"},
		{"code", "NOT_IN_MATCH_QUEUE"},
		{"message", "cancel match failed: session not in queue"}
	});
}

void RoomManager::createLessonRoom(std::shared_ptr<Session> session, const std::string& lessonId)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (session->getRoom()) {
		session->sendJson({
			{"type", "error"},
			{"code", "ALREADY_IN_ROOM"},
			{"message", "start lesson failed: already in room"}
		});
		return;
	}

	auto lesson = LessonController::findLesson(lessonId);
	if (!lesson.has_value()) {
		session->sendJson({
			{"type", "error"},
			{"code", "LESSON_NOT_FOUND"},
			{"message", "start lesson failed: lesson not found"}
		});
		return;
	}

	removeFromWaitingBucketsLocked(session);

	auto roomId = nextRoomId_++;
	auto room = std::make_shared<GameRoom>(io_, roomId, GameRoom::Mode::Lesson);
	rooms_[roomId] = room;

	auto player = std::make_shared<NetworkPlayer>(session, lesson->turn);
	session->setRoom(room);
	session->setPlayer(player);

	if (!room->startLesson(player, *lesson)) {
		session->clearRoom();
		session->setPlayer(nullptr);
		rooms_.erase(roomId);
		session->sendJson({
			{"type", "error"},
			{"code", "LESSON_INIT_FAILED"},
			{"message", "start lesson failed: lesson position init failed"}
		});
	}
}

void RoomManager::closeRoom(std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (!closeWaitingRoomLocked(session, false)) {
		session->sendJson({
			{"type", "error"},
			{"code", "ROOM_NOT_CLOSABLE"},
			{"message", "close room failed: session is not the waiting room host"}
		});
		return;
	}

	session->clearRoom();
	session->setPlayer(nullptr);

	session->sendJson({
		{"type", "room_closed"}
	});
}

void RoomManager::handleSessionClosed(const std::shared_ptr<Session>& session)
{
	std::lock_guard<std::mutex> lock(mutex_);

	removeFromWaitingBucketsLocked(session);
	closeWaitingRoomLocked(session, true);
}

std::string RoomManager::makeBucketKey(int initial, int increment)
{
	return std::to_string(initial) + "_" + std::to_string(increment);
}

bool RoomManager::removeFromWaitingBucketsLocked(const std::shared_ptr<Session>& session)
{
	bool removed = false;
	for (auto& item : waitingBuckets_) {
		auto& q = item.second;
		std::queue<std::shared_ptr<Session>> rebuilt;
		while (!q.empty()) {
			auto cur = q.front();
			q.pop();
			if (cur == session) {
				removed = true;
				continue;
			}
			if (cur && cur->isAlive()) {
				rebuilt.push(cur);
			}
		}
		q = std::move(rebuilt);
	}
	return removed;
}

bool RoomManager::closeWaitingRoomLocked(const std::shared_ptr<Session>& session, bool keepExpiredCode)
{
	auto room = session ? session->getRoom() : nullptr;
	if (!room)
		return false;

	const auto roomId = room->id();
	auto hostIt = roomHostPlayersById_.find(roomId);
	if (hostIt == roomHostPlayersById_.end() || !room->isEmpty())
		return false;

	auto hostNet = std::dynamic_pointer_cast<NetworkPlayer>(hostIt->second);
	if (!hostNet || hostNet->getSession() != session)
		return false;

	auto codeIt = roomCodesById_.find(roomId);
	if (keepExpiredCode && codeIt != roomCodesById_.end()) {
		expiredRoomCodes_[codeIt->second] = SteadyClock::now();
	}

	eraseRoomMappingsLocked(roomId);
	waitingRoomCreatedAtById_.erase(roomId);
	rooms_.erase(roomId);
	return true;
}

std::string RoomManager::generateRoomCodeLocked() const
{
	static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
	static constexpr std::size_t kCodeLen = 6;
	thread_local std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<std::size_t> dist(0, sizeof(alphabet) - 2);

	for (int attempt = 0; attempt < 32; ++attempt) {
		std::string code;
		code.reserve(kCodeLen);
		for (std::size_t i = 0; i < kCodeLen; ++i) {
			code.push_back(alphabet[dist(rng)]);
		}
		if (roomIdByCode_.find(code) == roomIdByCode_.end()) {
			return code;
		}
	}

	return "ROOM" + std::to_string(nextRoomId_.load());
}

void RoomManager::purgeExpiredWaitingRoomsLocked()
{
	const auto now = SteadyClock::now();
	for (auto it = waitingRoomCreatedAtById_.begin(); it != waitingRoomCreatedAtById_.end();) {
		if (now - it->second <= kWaitingRoomTimeout) {
			++it;
			continue;
		}

		const auto roomId = it->first;
		auto codeIt = roomCodesById_.find(roomId);
		if (codeIt != roomCodesById_.end()) {
			expiredRoomCodes_[codeIt->second] = now;
		}
		auto roomIt = rooms_.find(roomId);
		if (roomIt != rooms_.end()) {
			rooms_.erase(roomIt);
		}
		eraseRoomMappingsLocked(roomId);
		it = waitingRoomCreatedAtById_.erase(it);
	}
}

void RoomManager::purgeExpiredRoomCodeCacheLocked()
{
	const auto now = SteadyClock::now();
	for (auto it = expiredRoomCodes_.begin(); it != expiredRoomCodes_.end();) {
		if (now - it->second > kExpiredCodeRetention) {
			it = expiredRoomCodes_.erase(it);
		}
		else {
			++it;
		}
	}
}

void RoomManager::eraseRoomMappingsLocked(GameRoom::RoomID roomId)
{
	auto codeIt = roomCodesById_.find(roomId);
	if (codeIt != roomCodesById_.end()) {
		roomIdByCode_.erase(codeIt->second);
		roomCodesById_.erase(codeIt);
	}
	roomHostPlayersById_.erase(roomId);
}
