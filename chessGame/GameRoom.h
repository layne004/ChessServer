#pragma once
#include "Piece.h"
#include "Board.h"
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <json.hpp>
using json = nlohmann::json;

class Session;

class GameRoom:public std::enable_shared_from_this<GameRoom>
{
public:
	GameRoom(boost::asio::io_context& io);
	~GameRoom() = default;

	// 开始对战
	void start(std::shared_ptr<Session> p1, std::shared_ptr<Session> p2);

	// 玩家加入（按顺序）
	void join(std::shared_ptr<Session> player);

	// 玩家离开
	void leave(const std::shared_ptr<Session>& player);

	// 获取玩家颜色
	Color getPlayerColor(const std::shared_ptr<Session>& player) const;
	
	// 处理玩家走棋
	void handleMove(std::shared_ptr<Session> playerSession, Color player, const std::string& from, const std::string& to);

	// 处理玩家主动认输
	void handleResign(const std::shared_ptr<Session> player);

	// 获取当前轮
	Color currentTurn()const { return turn_; }

	// 获取房间状态
	bool isFull() const;
	bool isEmpty() const;

	// 重置游戏
	void resetGame();

private:
	void broadcast(const std::string& msg);
	void broadcastJson(const json& j);
	void broadcastState();
	void broadcastGameOver();

private:
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	Board board_;
	Color turn_ = Color::White; //白方先行

	std::weak_ptr<Session> p1_;
	std::weak_ptr<Session> p2_;

	int halfmoveClock_ = 0;
	int fullmoveNumber_ = 1;
};

