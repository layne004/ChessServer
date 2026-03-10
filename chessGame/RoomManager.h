#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <boost/asio.hpp>
class Session;
class GameRoom;

class RoomManager
{
public:
	RoomManager(boost::asio::io_context& io);

	// 处理匹配
	void handleMatch(std::shared_ptr<Session> session, const std::string& mode);

	void cleanupRooms();

private:
	void matchPvp(std::shared_ptr<Session> session);
	void createPveRoom(std::shared_ptr<Session> session);

private:
	boost::asio::io_context& io_; //保存引用
	std::vector<std::shared_ptr<GameRoom>> rooms_;
	std::atomic<int> nextRoomId_ = 1;
	std::queue<std::shared_ptr<Session>> waitingPvp_;
	std::mutex mutex_;
};

