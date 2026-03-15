#pragma once
#include "Piece.h"
#include "Board.h"
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <json.hpp>
using json = nlohmann::json;

class Session;

class Player;

enum class GameState {
	Waiting,
	Playing,
	GameOver
};

class GameRoom:public std::enable_shared_from_this<GameRoom>
{
public:
	using RoomID = uint64_t;
	GameRoom(boost::asio::io_context& io, RoomID roomId);
	~GameRoom() = default;

	// 开始对战
	void start(std::shared_ptr<Player> white, std::shared_ptr<Player> black);

	// 终止游戏
	void endGame(const std::string& result, const std::string& reason);
	
	// 处理玩家走棋
	void handleMove(std::shared_ptr<Player> player, const std::string& from, const std::string& to);

	// 处理AI玩家走棋
	void maybeAIMove();

	// 处理玩家主动认输
	void handleResign(const std::shared_ptr<Player> player);

	// 处理玩家Session断线
	void onPlayerDisconnected(const std::shared_ptr<Session>& session);

	// 断线重连
	void reconnect(const std::shared_ptr<Session>& session);

	// 已知Session找player
	std::shared_ptr<Player> findPlayer(const std::shared_ptr<Session>& session);

	// 获取当前轮
	Color currentTurn()const { return turn_; }

	// 获取房间状态
	bool isFull() const;
	bool isEmpty() const;
	// 获取房间号
	RoomID id()const { return roomId_; }

	// 重置游戏
	void resetGame();

private:
	void broadcast(const std::string& msg);
	void broadcastJson(const json& j);
	void broadcastMove(const std::string& from, const std::string& to);
	void broadcastState();
	//void broadcastGameOver(const std::string& result, const std::string& reason);

private:
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	Board board_;
	Color turn_ = Color::White; //白方先行
	RoomID roomId_;

	std::shared_ptr<Player> white_;
	std::shared_ptr<Player> black_;
	std::vector<std::shared_ptr<Player>> spectators_;

	GameState state_ = GameState::Waiting;

	int halfmoveClock_ = 0;
	int fullmoveNumber_ = 1;
};

