#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <boost/asio.hpp>
#include <unordered_map>
#include "GameRoom.h"
class Session;
class Player;


class RoomManager
{
public:
	RoomManager(boost::asio::io_context& io);

	// 处理匹配
	void handleMatch(std::shared_ptr<Session> session, const std::string& mode, 
		const std::string& level, const std::string& color, int initial, int increment);

	void cleanupRooms();

	void handleReconnect(std::shared_ptr<Session> s, GameRoom::RoomID roomId, const std::string& playerId);
	void createRoom(std::shared_ptr<Session> session, int initial, int increment);
	void joinRoom(std::shared_ptr<Session> session, const std::string& roomCode);
	void cancelMatch(std::shared_ptr<Session> session);

private:
	void matchPvp(std::shared_ptr<Session> session, int initial, int increment);
	void createPveRoom(std::shared_ptr<Session> session, const std::string& level, const std::string& color);
	void createLessonRoom();
	std::string makeBucketKey(int initial, int increment);
	bool removeFromWaitingBucketsLocked(const std::shared_ptr<Session>& session);
	std::string generateRoomCodeLocked() const;

private:
	boost::asio::io_context& io_; //保存引用
	std::unordered_map<GameRoom::RoomID, std::shared_ptr<GameRoom>> rooms_;
	std::unordered_map<GameRoom::RoomID, std::string> roomCodesById_;
	std::unordered_map<std::string, GameRoom::RoomID> roomIdByCode_;
	std::unordered_map<GameRoom::RoomID, std::shared_ptr<Player>> roomHostPlayersById_;
	std::atomic<int> nextRoomId_ = 1;
	std::unordered_map<std::string, std::queue<std::shared_ptr<Session>>> waitingBuckets_;
	std::mutex mutex_;
};

