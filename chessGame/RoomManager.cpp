#include "RoomManager.h"
#include "Session.h"
#include "GameRoom.h"
#include "NetworkPlayer.h"

RoomManager::RoomManager(boost::asio::io_context& io)
	:io_(io)
{
}

void RoomManager::leaveRoom(const std::shared_ptr<Session>& player)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto room = player->getRoom();
	if (!room)
		return;

	room->leave(player);
	player->clearRoom();

	// 房间一空就删除
	// todo 房间池
	if (room->isEmpty())
	{
		auto it = std::find(rooms_.begin(), rooms_.end(), room);
		if (it != rooms_.end())
		{
			rooms_.erase(it);
		}
	}
}

void RoomManager::handleMatch(std::shared_ptr<Session> session, const std::string& mode)
{
	if (mode == "pvp") {
		matchPvp(session);
	}
	else if (mode == "pve")
	{
		createPveRoom(session);
	}
}

void RoomManager::matchPvp(std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(mutex_);

	// 如果已经在匹配队列就拒绝
	if (session->getRoom())
		return;

	if (waitingPvp_.empty()) {
		waitingPvp_.push(session);

		session->sendJson({
			{"type", "matching"},
			{"mode", "pvp"}
		});
	}
	else {
		auto opponent = waitingPvp_.front();
		waitingPvp_.pop();

		auto room = std::make_shared<GameRoom>(io_);

		session->setRoom(room);
		opponent->setRoom(room);

		auto white = std::make_shared<NetworkPlayer>(opponent, Color::White);
		auto black = std::make_shared<NetworkPlayer>(session, Color::Black);

		opponent->setPlayer(white);
		session->setPlayer(black);

		room->start(white, black);
	}
}

void RoomManager::createPveRoom(std::shared_ptr<Session> session)
{
}
