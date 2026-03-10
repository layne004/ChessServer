#include "RoomManager.h"
#include "Session.h"
#include "GameRoom.h"
#include "NetworkPlayer.h"

RoomManager::RoomManager(boost::asio::io_context& io)
	:io_(io)
{
}

void RoomManager::cleanupRooms()
{
	std::lock_guard<std::mutex> lock(mutex_);

	rooms_.erase(
		std::remove_if(
			rooms_.begin(),
			rooms_.end(),
			[](const std::shared_ptr<GameRoom>& room)
			{
				return room->isEmpty();
			}
		),
		rooms_.end()
	);
}

void RoomManager::handleMatch(std::shared_ptr<Session> session, const std::string& mode)
{
	cleanupRooms();

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

		int roomId = nextRoomId_++;
		auto room = std::make_shared<GameRoom>(io_, roomId);
		rooms_.push_back(room);

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
