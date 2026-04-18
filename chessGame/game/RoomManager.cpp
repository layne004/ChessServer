#include "RoomManager.h"
#include "Session.h"
#include "NetworkPlayer.h"
#include "AIPlayer.h"

RoomManager::RoomManager(boost::asio::io_context& io)
	:io_(io)
{
}

void RoomManager::cleanupRooms()
{
	std::lock_guard<std::mutex> lock(mutex_);

	for (auto it = rooms_.begin(); it != rooms_.end();)
	{
		if (it->second->isEmpty())
			it = rooms_.erase(it);
		else
			++it;
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

void RoomManager::createLessonRoom()
{

}

std::string RoomManager::makeBucketKey(int initial, int increment)
{
	return std::to_string(initial) + "_" + std::to_string(increment);
}
